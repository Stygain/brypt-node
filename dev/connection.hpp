#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <unistd.h>
#include <vector>
#include <fstream>

#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "zmq.hpp"

#include "utility.hpp"
#include "message.hpp"
#include "lora.hpp"

// TODO:
// * Drop connections if the connected node does not match the intended device
// * Maintain key and nonce state for connections

class direct_monitor_t : public zmq::monitor_t {
    public:
        bool connected = false;
        bool disconnected = false;

        void on_event_connected(const zmq_event_t &, const char *) ZMQ_OVERRIDE {
            this->connected = true;
        }

        void on_event_closed(const zmq_event_t &, const char *) ZMQ_OVERRIDE {
            this->disconnected = true;
        }

        void on_event_disconnected(const zmq_event_t &, const char *) ZMQ_OVERRIDE {
            this->disconnected = true;
        }
};

class Connection {
    protected:
    	bool active;
    	bool instantiate_connection;
    	DeviceOperation operation;

    	std::string peer_name;
    	std::string pipe_name;
    	std::fstream pipe;
    	unsigned long message_sequence;

        SystemClock update_clock;

        bool worker_active = false;
        bool response_needed = false;
        std::thread worker_thread;
        std::mutex worker_mutex;
        std::condition_variable worker_conditional;

    public:
    	virtual void whatami() = 0;
    	virtual void spawn() = 0;
    	virtual void worker() = 0;
        virtual std::string get_protocol_type() = 0;
        virtual std::string get_internal_type() = 0;
    	virtual std::string recv(int flag = 0) = 0;
    	virtual void send(Message *) = 0;
    	virtual void send(const char * message) = 0;
        virtual void prepare_for_next() = 0;
    	virtual void shutdown() = 0;

    	bool get_status() {
    	    return this->active;
    	}

    	bool get_worker_status() {
    	    return this->worker_active;
    	}

        std::string get_peer_name() {
            return this->peer_name;
        }

        std::string get_pipe_name() {
            return this->pipe_name;
        }

        SystemClock get_update_clock() {
            return this->update_clock;
        }

    	bool create_pipe() {
    	    if (this->peer_name == "") {
                return false;
    	    }

    	    std::string filename = "./tmp/" + this->peer_name + ".pipe";

    	    this->pipe_name = filename;
    	    this->pipe.open(filename, std::fstream::in | std::fstream::out | std::fstream::trunc);

    	    if( this->pipe.fail() ){
        		this->pipe.close();
        		return false;
    	    }

    	    return true;
    	}

    	bool write_to_pipe(std::string message) {
    	    if ( !this->pipe.good() ) {
                return false;
    	    }

            printo("Writing \"" + message + "\" to pipe", CONNECTION_P);
    	    this->pipe.clear();
    	    this->pipe.seekp(0);
    	    this->pipe << message << std::endl;
    	    this->pipe.flush();
    	    return true;
    	}

    	std::string read_from_pipe() {
    	    std::string raw_message = "";

            if ( !this->pipe.good() ) {
                printo("Pipe file is not good", CONNECTION_P);
                return raw_message;
            }
    	    this->pipe.clear();
    	    this->pipe.seekg(0);

            if (this->pipe.eof()) {
                printo("Pipe file is at the EOF", CONNECTION_P);
                return raw_message;
            }

            std::getline( this->pipe, raw_message );
            this->pipe.clear();

            printo("Sending " + raw_message, CONNECTION_P);

            std::ofstream clear_file(pipe_name, std::ios::out | std::ios::trunc);
            clear_file.close();

            return raw_message;
        }

        void response_ready(std::string id) {
            if (this->peer_name != id) {
                printo("Response was not for this peer", CONNECTION_P);
                return;
            }

            this->response_needed = false;
            std::unique_lock<std::mutex> thread_lock(this->worker_mutex);
            worker_conditional.notify_one();
            thread_lock.unlock();
        }

        void unspecial() {
            printo("I am calling an unspecialized function", CONNECTION_P);
        }

};

class Direct : public Connection {
    private:
    	bool control;

    	std::string port;
    	std::string peer_addr;
    	std::string peer_port;

    	zmq::context_t *context;
    	zmq::socket_t *socket;
    	zmq::pollitem_t item;

    public:
        Direct() {}
        Direct(struct Options *options) {
	    printo("[Direct] Creating direct instance", CONNECTION_P);

            this->port = options->port;
            this->peer_name = options->peer_name;
            this->peer_addr = options->peer_addr;
            this->peer_port = options->peer_port;
            this->control = options->is_control;
            this->operation = options->operation;

            this->update_clock = get_system_clock();

    	    if (options->is_control) {
			printo("[Direct] Creating control socket", CONNECTION_P);

        		this->context = new zmq::context_t(1);

        		switch (options->operation) {
                    case ROOT: {
			printo("[Direct] Setting up REP socket on port " + options->port, CONNECTION_P);
                        this->setup_rep_socket(options->port);
                        break;
                    }
                    case BRANCH: {
                        break;
                    }
                    case LEAF: {
			printo("[Direct] Connecting REQ socket to " + options->peer_addr, CONNECTION_P);
                        this->setup_req_socket(options->peer_addr, options->peer_port);
                        break;
                    }
                    case NO_OPER: {
			printo("Error: Device operation needed", ERROR_P);
                        exit(0);
                        break;
                    }
        		}

        		return;

    	    }

            this->spawn();

            std::unique_lock<std::mutex> thread_lock(worker_mutex);
            this->worker_conditional.wait(thread_lock, [this]{return this->worker_active;});
            thread_lock.unlock();

            // CHECK WORKER ACTIVE FROM CALLING FUNCTION

        }

        void whatami() {
            printo("[Direct] I am a Direct implementation", CONNECTION_P);
        }

        void spawn() {
            printo("[Direct] Spawning DIRECT_TYPE connection thread", CONNECTION_P);
            this->worker_thread = std::thread(&Direct::worker, this);
        }

    	std::string get_protocol_type() {
    	    return "WiFi";
    	}

    	std::string get_internal_type() {
    	    return "Direct";
    	}

        void worker() {
            this->worker_active = true;
            this->create_pipe();

            // Create appriopiate network socket
            this->context = new zmq::context_t(1);

            switch (this->operation) {
                case ROOT: {
                    printo("[Direct] Setting up REP socket on port " + this->port, CONNECTION_P);
                    this->setup_rep_socket(this->port);
                    // handle_messaging();
                    break;
                }
                case BRANCH: {
                    break;
                }
                case LEAF: {
                    printo("[Direct] Connecting REQ socket to " + this->peer_addr + ":" + this->peer_port, CONNECTION_P);
                    this->setup_req_socket(this->peer_addr, this->peer_port);

                    break;
                }
                case NO_OPER: {
                    printo("Error: Device operation needed", ERROR_P);
                    exit(0);
                    break;
                }
            }

            this->worker_active = true;

            // Notify the calling thread that the connection worker is ready
            std::unique_lock<std::mutex> thread_lock(this->worker_mutex);
            worker_conditional.notify_one();
            thread_lock.unlock();

            unsigned int run = 0;

            do {
                std::string request = "";
                // Receive message
                request = this->recv(0);
                printo("[Direct] Received request: " + request, CONNECTION_P);
                this->write_to_pipe(request);

                // std::this_thread::sleep_for(std::chrono::milliseconds(500));

                this->response_needed = true;

                // Wait for message from pipe then send
                std::unique_lock<std::mutex> thread_lock(worker_mutex);
                this->worker_conditional.wait(thread_lock, [this]{return !this->response_needed;});
                thread_lock.unlock();

                std::string response = this->read_from_pipe();
                this->send(response.c_str());

                // Message response("1", this->peer_name, QUERY_TYPE, 1, "Message Response", run);
                // this->send(&response);

                run++;
                std::this_thread::sleep_for(std::chrono::nanoseconds(1000));
            } while(true);

        }

        void setup_rep_socket(std::string port) {
            this->socket = new zmq::socket_t(*this->context, ZMQ_REP);
            this->item.socket = *this->socket;
            this->item.events = ZMQ_POLLIN;
            this->instantiate_connection = true;
            this->socket->bind("tcp://*:" + port);
        }

        void setup_req_socket(std::string addr, std::string port) {
            this->socket = new zmq::socket_t(*this->context, ZMQ_REQ);
            this->item.socket = *this->socket;
            this->item.events = ZMQ_POLLIN;
            this->socket->connect("tcp://" + addr + ":" + port);
            this->instantiate_connection = false;
        }

    	void send(class Message * message) {
    	    std::string msg_pack = message->get_pack();
    	    zmq::message_t request(msg_pack.size());
    	    memcpy(request.data(), msg_pack.c_str(), msg_pack.size());

            this->socket->send(request);
            this->message_sequence++;

            printo("[Direct] Sent: (" + std::to_string(strlen(msg_pack.c_str())) + ") " + msg_pack, CONNECTION_P);
	    }

    	void send(const char * message) {
    	    zmq::message_t request(strlen(message));
    	    memcpy(request.data(), message, strlen(message));
    	    this->socket->send(request);
                this->message_sequence++;

    	    printo("[Direct] Sent: (" + std::to_string(strlen(message)) + ") " + message, CONNECTION_P);
    	}

    	std::string recv(int flag){
    	    zmq::message_t message;
    	    this->socket->recv(&message, flag);
    	    std::string request = std::string(static_cast<char *>(message.data()), message.size());

    	    if (request != "") {
                this->update_clock = get_system_clock();
                this->message_sequence++;
    	    }

    	    printo("[Direct] Received: " + request, CONNECTION_P);
    	    return request;
    	}

    	void prepare_for_next() {

    	}

    	void shutdown() {
    	    printo("[Direct] Shutting down socket and context", CONNECTION_P);
    	    zmq_close(this->socket);
    	    zmq_ctx_destroy(this->context);
    	}

    	void handle_messaging() {

    	}
};

class StreamBridge : public Connection {
	private:
		bool control;

		std::string port;
		std::string peer_addr;
		std::string peer_port;

		int init_msg = 1;

		void *context;
		void *socket;

		uint8_t id[256];
		size_t id_size = 256;

	public:
		StreamBridge() {}
		StreamBridge(struct Options *options) {
			printo("Creating StreamBridge instance", CONNECTION_P);

			this->port = options->port;
			this->peer_name = options->peer_name;
			this->peer_addr = options->peer_addr;
			this->peer_port = options->peer_port;
			this->control = options->is_control;
			this->operation = options->operation;

			// this->worker_active = false;
			this->update_clock = get_system_clock();

			if (options->is_control) {
				printo("[StreamBridge] creating control socket", CONNECTION_P);

				this->context = zmq_ctx_new();// add 1 or no

				switch (options->operation) {
					case ROOT: {
							   printo("[StreamBridge] Setting up StreamBridge socket on port " + options->port, CONNECTION_P);
							   this->socket = zmq_socket(this->context, ZMQ_STREAM);
							   this->init_msg = 1;
							   setup_streambridge_socket(options->port);
							   break;
						   }
					case BRANCH: {
							     break;
						     }
					case LEAF: {
							   //printo("[StreamBridge] connecting to StreamBridge socket " + options->peer_addr + ":" + options->peer_port, CONNECTION_P);
							   //this->socket = zmq_socket(this->context, ZMQ_STREAM);
							   //setup_streambridge_socket(options->port);
							   //break;
						   }
					case NO_OPER: {
							      printo("Error: Device operation needed", ERROR_P);
							      exit(0);
							      break;
						      }
				}
				return;
			}

			this->spawn();

			std::unique_lock<std::mutex> thread_lock(worker_mutex);
			this->worker_conditional.wait(thread_lock, [this]{return this->worker_active;});
			thread_lock.unlock();
		}
// <<<<<<< HEAD
// 		return;
// 	    }

// 	    this->spawn();

// 	    std::unique_lock<std::mutex> thread_lock(worker_mutex);
// 	    this->worker_conditional.wait(thread_lock, [this]{return this->worker_active;});
// 	    thread_lock.unlock();
// 	}

// 	void whatami() {
// 	    std::cout << "I am a StreamBridge implementation." << '\n';
// 	}

//     std::string get_type() {
//         return "WiFi";
//     }

// 	void spawn() {
// 	    std::cout << "== [StreamBridge] Spawning STREAMBRIDGE_TYPE connection thread\n";
// 	    this->worker_thread = std::thread(&StreamBridge::worker, this);
// 	}

//     std::string get_internal_type() {
//         return "StreamBridge";
//     }


// 	void worker() {
// 	    this->worker_active = true;
// 	    this->create_pipe();

// 	    this->context = zmq_ctx_new();// add 1 or no

// 	    switch (this->operation) {
// 		case ROOT: {
// 				std::cout << "== [StreamBridge] setting up stream socket on port " << this->port << "\n";
// 				this->socket = zmq_socket(this->context, ZMQ_STREAM);
// 				this->init_msg = 1;
// 				setup_streambridge_socket(this->port);
// 				break;
// 			   }
// 		case BRANCH: {
// 				 break;
// 			     }
// 		case LEAF: {
// 				//std::cout << "== [StreamBridge] connecting stream socket to " << options->peer_addr << ":" << options->peer_port << "\n";
// 				//this->socket = zmq_socket(this->context, ZMQ_STREAM);
// 				//setup_streambridge_socket(options->port);
// 				//break;
// 			   }
//                case NO_OPER: {
//                    std::cout << "ERROR DEVICE OPERATION NEEDED" << "\n";
//                    exit(0);
//                    break;
//                }
// 	    }

// 	    this->worker_active = true;

// 	    // Notify the calling thread that the connection worker is ready
// 	    std::unique_lock<std::mutex> thread_lock(this->worker_mutex);
// 	    worker_conditional.notify_one();
// 	    thread_lock.unlock();
// =======
// >>>>>>> origin

		void whatami() {
		        printo("[StreamBridge] I am a StreamBridge implementation", CONNECTION_P);
		}

		void spawn() {
		        printo("[StreamBridge] Spawning STREAMBRIDGE_TYPE connection thread", CONNECTION_P);
			this->worker_thread = std::thread(&StreamBridge::worker, this);
		}

		std::string get_protocol_type() {
			return "WiFi";
		}

		std::string get_internal_type() {
			return "StreamBridge";
		}

		void worker() {
			this->worker_active = true;
			this->create_pipe();

			this->context = zmq_ctx_new();// add 1 or no

			switch (this->operation) {
				case ROOT: {
						   printo("[StreamBridge] Setting up StreamBridge socket on port " + this->port, CONNECTION_P);
						   this->socket = zmq_socket(this->context, ZMQ_STREAM);
						   this->init_msg = 1;
						   setup_streambridge_socket(this->port);
						   break;
					   }
				case BRANCH: {
						     break;
					     }
				case LEAF: {
						   //printo("[StreamBridge] Connecting StreamBridge socket to " + this->peer_addr + ":" + this->peer_port, CONNECTION_P);
						   //this->socket = zmq_socket(this->context, ZMQ_STREAM);
						   //setup_streambridge_socket(options->port);
						   //break;
					   }
				case NO_OPER: {
						      printo("Error: Device operation needed", ERROR_P);
						      exit(0);
						      break;
					      }
			}

			this->worker_active = true;

			// Notify the calling thread that the connection worker is ready
			std::unique_lock<std::mutex> thread_lock(this->worker_mutex);
			worker_conditional.notify_one();
			thread_lock.unlock();

			unsigned int run = 0;

			do {
				std::string request = "";
				// Receive message
				request = this->recv(0);
				this->write_to_pipe(request);

				//std::this_thread::sleep_for(std::chrono::milliseconds(500));

				this->response_needed = true;

				// Wait for message from pipe then send
				std::unique_lock<std::mutex> thread_lock(worker_mutex);
				this->worker_conditional.wait(thread_lock, [this]{return !this->response_needed;});
				thread_lock.unlock();

				std::string response = this->read_from_pipe();
				this->send(response.c_str());


				run++;
				std::this_thread::sleep_for(std::chrono::nanoseconds(1000));
			} while(true);
		}

		void setup_streambridge_socket(std::string port) {
		        printo("[StreamBridge] Setting up StreamBridge socket on port " + port, CONNECTION_P);
			this->instantiate_connection = true;
			std::string conn_data = "tcp://*:" + port;
			zmq_bind(this->socket, conn_data.c_str());
		}

		std::string recv(int flag){
			if (flag) {}
			char buffer[512];
			memset(buffer, '\0', 512);

			// Receive 4 times, first is ID, second is nothing, third is message
			this->id_size = zmq_recv(this->socket, this->id, 256, 0);
		        //printo("[StreamBridge] Received ID: " + this->id, CONNECTION_P);
			size_t msg_size = zmq_recv(this->socket, buffer, 512, 0);
			memset(buffer, '\0', 512);
			msg_size = zmq_recv(this->socket, buffer, 512, 0);
			memset(buffer, '\0', 512);
			msg_size = zmq_recv(this->socket, buffer, 512, 0);
		        printo("[StreamBridge] Received: " + std::string(buffer), CONNECTION_P);
			this->init_msg = 0;

			return buffer;
		}

		void send(class Message * msg) {
			int flag = 0;
			std::string to_cat = "";
			std::string msg_pack = msg->get_pack();
			zmq_send(this->socket, this->id, this->id_size, ZMQ_SNDMORE);
			if ((msg_pack.c_str())[strlen(msg_pack.c_str())] != ((char)4)) {
				flag = 1;
				to_cat = (char)4 + "";
			}
			zmq_send(this->socket, msg_pack.c_str(), strlen(msg_pack.c_str()), ZMQ_SNDMORE);
			if (flag) {
				zmq_send(this->socket, to_cat.c_str(), strlen(to_cat.c_str()), ZMQ_SNDMORE);
			}
		        printo("[StreamBridge] Sent: (" + std::to_string(strlen(msg_pack.c_str())) + ") " + msg_pack, CONNECTION_P);
		}

		void send(const char * message) {
			int flag = 0;
			std::string to_cat = "";
			if ((message)[strlen(message)] != ((char)4)) {
				flag = 1;
				to_cat = (char)4 + "";
			}
			zmq_send(this->socket, this->id, this->id_size, ZMQ_SNDMORE);
			zmq_send(this->socket, message, strlen(message), ZMQ_SNDMORE);
			if (flag) {
				zmq_send(this->socket, to_cat.c_str(), strlen(to_cat.c_str()), ZMQ_SNDMORE);
			}
			printo("[StreamBridge] Sent: (" + std::to_string(strlen(message)) + ") " + message, CONNECTION_P);
		}

		void prepare_for_next() {

		}

		void shutdown() {
			printo("[StreamBridge] Shutting down socket and context", CONNECTION_P);
			zmq_close(this->socket);
			zmq_ctx_destroy(this->context);
		}

};

class TCP : public Connection {
    private:
	bool control;

	std::string port;
	std::string peer_addr;
	std::string peer_port;

	int socket;
	int connection = -1;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

    public:
	TCP() {}
	TCP(struct Options *options) {
	    printo("Creating TCP instance", CONNECTION_P);

	    this->port = options->port;
            this->peer_name = options->peer_name;
	    this->peer_addr = options->peer_addr;
	    this->peer_port = options->peer_port;
	    this->control = options->is_control;
	    this->operation = options->operation;

	    // this->worker_active = false;

	    if (options->is_control) {
		switch (options->operation) {
		    case ROOT: {
				   printo("[TCP] Setting up TCP socket on port " + options->port, CONNECTION_P);
				   this->setup_tcp_socket(options->port);
				   break;
			       }
		    case BRANCH: {
				     break;
				 }
		    case LEAF: {
				   printo("[TCP] Connecting TCP client socket to " + options->peer_addr + ":" + options->peer_port, CONNECTION_P);
				   this->setup_tcp_connection(options->peer_addr, options->peer_port);
				   break;
			       }
		    case NO_OPER: {
			printo("Error: Device operation needed", ERROR_P);
			exit(0);
			break;
		    }
		}
		return;
	    }

	    this->spawn();

	    std::unique_lock<std::mutex> thread_lock(worker_mutex);
	    this->worker_conditional.wait(thread_lock, [this]{return this->worker_active;});
	    thread_lock.unlock();
	}

	void whatami() {
	    printo("[TCP] I am a TCP implementation", CONNECTION_P);
	}

	void spawn() {
	    printo("[TCP] Spawning TCP_TYPE connection thread", CONNECTION_P);
	    this->worker_thread = std::thread(&TCP::worker, this);
	}

	std::string get_protocol_type() {
	    return "WiFi";
	}

	std::string get_internal_type() {
	    return "TCP";
	}

	void worker() {
	    this->worker_active = true;
	    this->create_pipe();

	    switch (this->operation) {
		case ROOT: {
		    printo("[TCP] Setting up TCP socket on port " + this->port, CONNECTION_P);
		    this->setup_tcp_socket(this->port);
		    // handle_messaging();
		    break;
		}
		case BRANCH: {
		    break;
		}
		case LEAF: {
		    printo("[TCP] Connecting TCP client socket to " + this->peer_addr + ":" + this->peer_port, CONNECTION_P);
		    this->setup_tcp_connection(this->peer_addr, this->peer_port);

		    break;
		}
		case NO_OPER: {
		    printo("Error: Device operation needed", ERROR_P);
		    exit(0);
		    break;
		}
	    }

	    this->worker_active = true;

	    // Notify the calling thread that the connection worker is ready
	    std::unique_lock<std::mutex> thread_lock(this->worker_mutex);
	    worker_conditional.notify_one();
	    thread_lock.unlock();

	    unsigned int run = 0;

	    do {
		std::string request = "";
		// Receive message
		request = this->recv(0);
		this->write_to_pipe(request);

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		// Wait for message from pipe then send

		run++;
		std::this_thread::sleep_for(std::chrono::nanoseconds(1000));
	    } while(true);
	}

	void setup_tcp_socket(std::string port) {
	    // Creating socket file descriptor
	    if ((this->socket = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printo("[TCP] Socket failed", CONNECTION_P);
		return;
	    }

	    if (setsockopt(this->socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &this->opt, sizeof(this->opt))) {
		printo("[TCP] SetSockOpt failed", CONNECTION_P);
		return;
	    }
	    this->address.sin_family = AF_INET;
	    this->address.sin_addr.s_addr = INADDR_ANY;
	    int PORT = std::stoi(port);
	    this->address.sin_port = htons( PORT );

	    if (bind(this->socket, (struct sockaddr *)&this->address, sizeof(this->address))<0) {
		printo("[TCP] Bind failed", CONNECTION_P);
		return;
	    }
	    if (listen(this->socket, 30) < 0) {
		printo("[TCP] Listen failed", CONNECTION_P);
		return;
	    }
	    // Make the socket non-blocking
	    if (fcntl(this->socket, F_SETFL, fcntl(this->socket, F_GETFL) | O_NONBLOCK) < 0) {
		return;
	    }
	}

	void setup_tcp_connection(std::string peer_addr, std::string peer_port) {
	    if ((this->connection = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return;
	    }

	    memset(&this->address, '0', sizeof(this->address));

	    this->address.sin_family = AF_INET;
	    int PORT = std::stoi(peer_port);
	    this->address.sin_port = htons(PORT);

	    // Convert IPv4 and IPv6 addresses from text to binary form
	    if (inet_pton(AF_INET, peer_addr.c_str(), &(this->address.sin_addr)) <= 0) {
		return;
	    }

	    if (connect(this->connection, (struct sockaddr *)&(this->address), sizeof(this->address)) < 0) {
		return;
	    }
	}

	std::string recv(int flag){
	    if (this->connection == -1) {
		if (flag != ZMQ_NOBLOCK) {
		    if (fcntl(this->socket, F_SETFL, fcntl(this->socket, F_GETFL) & (~O_NONBLOCK)) < 0) {
			return "";
		    }
		}
		this->connection = accept(this->socket, (struct sockaddr *)&this->address, (socklen_t*)&this->addrlen);
		if (fcntl(this->socket, F_SETFL, fcntl(this->socket, F_GETFL) | O_NONBLOCK) < 0) {
		    return "";
		}
	    }
	    if (flag == ZMQ_NOBLOCK) {
		if (fcntl(this->connection, F_SETFL, fcntl(this->connection, F_GETFL) | O_NONBLOCK) < 0) {
		    return "";
		}
	    } else {
		if (fcntl(this->connection, F_SETFL, fcntl(this->connection, F_GETFL) & (~O_NONBLOCK)) < 0) {
		    return "";
		}
	    }
	    char buffer[1024];
	    memset(buffer, '\0', 1024);
	    int valread = read(this->connection, buffer, 1024);
	    printo("[TCP] Received: (" + std::to_string((int)valread) + ") " + std::string(buffer), CONNECTION_P);

	    return std::string(buffer, strlen(buffer));
	}

	std::string internal_recv() {
	    char buffer[1024];
	    memset(buffer, '\0', 1024);
	    int valread = read(this->connection, buffer, 1024);
	    printo("[TCP] Received: (" + std::to_string((int)valread) + ") " + std::string(buffer), CONNECTION_P);

	    return std::string(buffer, strlen(buffer));
	}

	void send(Message * msg) {
	    std::string msg_pack = msg->get_pack();
	    ssize_t bytes = ::send(this->connection, msg_pack.c_str(), strlen(msg_pack.c_str()), 0);
	    printo("[TCP] Sent: (" + std::to_string((int)bytes) + ") " + msg_pack, CONNECTION_P);
	}

	void send(const char * message) {
	    ssize_t bytes = ::send(this->connection, message, strlen(message), 0);
	    printo("[TCP] Sent: (" + std::to_string((int)bytes) + ") " + std::string(message), CONNECTION_P);
	}

	void prepare_for_next() {
	    close(this->connection);
	    this->connection = -1;
	}

	void shutdown() {
	    close(this->connection);
	    close(this->socket);
	}

};


class Bluetooth : public Connection {
    public:
	void whatami() {
	    printo("[BLE] I am a BLE implementation", CONNECTION_P);
	}

	void spawn() {

	}

	std::string get_protocol_type() {
	    return "BLE";
	}

	std::string get_internal_type() {
	    return "BLE";
	}

	void worker() {

	}

	void send(class Message * msg) {
	    msg->get_pack();
	}

	void send(const char * message) {
	    printo("[BLE] Sent: " + std::string(message), CONNECTION_P);
	}

	std::string recv(int flag){
        if (flag) {}

	    return "Message";
	}

	void prepare_for_next() {

	}

	void shutdown() {

	}
};

class LoRa : public Connection {
    private:
        std::string port;
        std::string peer_addr;
        std::string peer_port;
        std::string id;

    public:
    	LoRa() {}
    	LoRa(struct Options *options){
            std::cout << "== [Connection] Creating LoRa instance.\n";

            this->port = options->port;
            this->peer_name = options->peer_name;
            this->peer_addr = options->peer_addr;
            this->peer_port = options->peer_port;
            this->operation = options->operation;
            this->id = options->id;

            this->spawn();

            std::unique_lock<std::mutex> thread_lock(worker_mutex);
            this->worker_conditional.wait(thread_lock, [this]{return this->worker_active;});
            thread_lock.unlock();
    	}

    	void whatami() {
    	    std::cout << "I am a LoRa implementation." << '\n';
    	}

    	std::string get_protocol_type() {
    	    return "LoRa";
    	}

    	std::string get_internal_type() {
    	    return "LoRa";
    	}

    	void spawn() {
            std::cout << "== [LoRa] Spawning LORA_TYPE connection thread\n";
            this->worker_thread = std::thread(&LoRa::worker, this);
    	}

    	void worker() {
            this->worker_active = true;
            this->create_pipe();
            // LoRa initial setup
            wiringPiSetup () ;
            pinMode(ssPin, OUTPUT);
            pinMode(dio0, INPUT);
            pinMode(RST, OUTPUT);

            wiringPiSPISetup(CHANNEL, 500000);

            bool sx1272 = SetupLoRa();

            opmodeLora(sx1272);
            opmode(OPMODE_STANDBY);
            writeReg(RegPaRamp, (readReg(RegPaRamp) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec
            configPower(23, sx1272);

            // Notify the calling thread that the connection worker is ready
            std::unique_lock<std::mutex> thread_lock(this->worker_mutex);
            worker_conditional.notify_one();
            thread_lock.unlock();

            unsigned int run = 0;

            clock_t send_diff = clock();

            opmode(OPMODE_STANDBY);
            opmode(OPMODE_RX);

            do {
                // Wait for message from pipe then send
                // std::unique_lock<std::mutex> thread_lock(worker_mutex);
                // this->worker_conditional.wait(thread_lock, [this]{return !this->response_needed;});
                // thread_lock.unlock();
                // Send message
                if(((clock() - send_diff)/CLOCKS_PER_SEC) >= 5){
                    opmode(OPMODE_STANDBY);
                    // std::string response = "HELLO";
                    // this->send(response.c_str());
                    Message response("1", this->peer_name, QUERY_TYPE, 1, "HELLO", run);
                    this->send(&response);
                    send_diff = clock();
                    opmode(OPMODE_STANDBY);
                    opmode(OPMODE_RX);
                }

                // Receive message
                std::string received = "";
                received = this->recv((int)sx1272);
                if(received.size() >= 48){
                    received.erase(48, received.size()-1);
                    Message message(received);
                }
                std::cout << "== [LoRa] received: " << received << '\n';
                //this->write_to_pipe(received);

                //std::this_thread::sleep_for(std::chrono::milliseconds(500));

                //this->response_needed = true;

                

                run++;
                //std::this_thread::sleep_for(std::chrono::nanoseconds(5000));
            } while(true);
    	}

    	void send(class Message * msg) {
    	    std::cout << "[LoRa] Sending..." << '\n';
            std::string msg_pack = msg->get_pack();
            byte* byte_message = (unsigned char*)msg_pack.c_str();
            txlora(byte_message, strlen((char*)byte_message));
            clock_t t = clock();
            while(((clock() - t)/CLOCKS_PER_SEC) < 0.5);
            std::cout << "[LoRa] Sent: (" << strlen(msg_pack.c_str()) << ") " << msg_pack << '\n';
    	}

    	void send(const char * message) {
            byte* byte_message = (unsigned char*)message;
    	    std::cout << "[LoRa] Sending..." << '\n';
            txlora(byte_message, strlen(message));
            clock_t t = clock();
            while(((clock() - t)/CLOCKS_PER_SEC) < 0.5);
            std::cout << "[LoRa] Sent: (" << strlen(message) << ") " << message << '\n';
    	}

    	std::string recv(int flag){
            std::string received = "";
            clock_t t = clock();
            while(((clock() - t)/CLOCKS_PER_SEC) < 0.5);
            received += receivepacket((bool)flag);

            return received;
    	}

        // void send(const char * message) {
        //     printo("[LoRa] Sent: " + std::string(message), CONNECTION_P);
        // }

        void prepare_for_next() {

        }

    	void shutdown() {

    	}
};

inline Connection* ConnectionFactory(TechnologyType technology) {
    switch (technology) {
	case DIRECT_TYPE:
	    return new Direct;
	    break;
	case BLE_TYPE:
	    return new Bluetooth;
	    break;
	case LORA_TYPE:
	    return new LoRa;
	    break;
	case TCP_TYPE:
	    return new TCP;
	    break;
	case STREAMBRIDGE_TYPE:
	    return new StreamBridge;
	    break;
	case NO_TECH:
	    return NULL;
	    break;
    }
    return NULL;
}

inline Connection* ConnectionFactory(TechnologyType technology, Options *options) {
    switch (technology) {
	case DIRECT_TYPE:
	    return new Direct(options);
	    break;
	case BLE_TYPE:
	    return new Bluetooth();
	    break;
	case LORA_TYPE:
	    return new LoRa(options);
	    break;
	case TCP_TYPE:
	    return new TCP(options);
	    break;
	case STREAMBRIDGE_TYPE:
	    return new StreamBridge(options);
	    break;
	case NO_TECH:
	    return NULL;
	    break;
    }
    return NULL;
}

#endif
