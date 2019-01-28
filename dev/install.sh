#!/bin/bash
# Check to see if correct usage
if [ "$#" -ne 1 ]
	then
		echo "USAGE: ./install.sh [administrator_password]"
		exit 1
fi
# Install ZMQ
# Start with necessary packages
echo $1 | sudo -S apt-get install libtool pkg-config build-essential autoconf automake git
# Download ZMQ package
wget http://download.zeromq.org/zeromq-4.1.4.tar.gz
tar -zxvf zeromq-4.1.4.tar.gz
cd zeromq-4.1.4/
./configure --without-libsodium
make
sudo make install
sudo ldconfig
# Make CPP bindings
cd ~
git clone https://github.com/zeromq/cppzmq.git
sudo cp cppzmq/zmq.hpp /usr/local/include/
# Get and compile brypt-node
cd ~
git clone https://github.com/Stygain/brypt-node.git
cd brypt-node/dev
g++ node.cpp "test.cpp" -o device -O2 -Wall -lzmq
./device --server -type DIRECT -port 3005
