#include <SPI.h>
#include <WiFi101.h>

#include "message.hpp"
#include "utility.hpp"

const int NETWORK_LIST_SIZE = 20;
const int AP_NAME_LEN = 40;

// Enum to keep track of current state
enum CurrentState { DETERMINE_NETWORK, AP_DETERMINED, AP_CONNECTED, SERVER_CONNECTED };

int builtin_led =  LED_BUILTIN;

// Keep track of the current wifi status
int wifi_status = WL_IDLE_STATUS;

// Server port to create for the web portal
WiFiServer server(80);

void listNetworks();
void printWiFiStatus();
void start_access_point(char ssid[]);
int check_wifi_status(int);
void scan_available_networks();
void send_http_response(WiFiClient);
String handle_form_submission(WiFiClient, String);
void printEncryptionType(int);
void printMacAddress(byte mac[]);

char AP_names[NETWORK_LIST_SIZE][AP_NAME_LEN];
String access_point;

// Initial state on setup is to determine the network
CurrentState state = DETERMINE_NETWORK;
int contacted = 0;

// CONFIG IP address of server to connect to
IPAddress remote_server(192,168,137,135);
int remote_port = 3001;
String device_id = "1998";

WiFiClient server_connection;




/* **************************************************************************
** Function: base64_encode
** Description: Encode a std::string to a Base64 message
** Source: https://github.com/ReneNyffenegger/cpp-base64/blob/master/base64.cpp#L45
** *************************************************************************/
String base64_encode(String message, unsigned int in_len) {
  String encoded;
  int idx = 0, jdx = 0;
  unsigned char char_array_3[3], char_array_4[4];
  unsigned char const * bytes_to_encode = reinterpret_cast<const unsigned char *>( message.c_str() );

  while (in_len--) {
    char_array_3[idx++] = *(bytes_to_encode++);

    if(idx == 3) {
      char_array_4[0] = ( char_array_3[0] & 0xfc ) >> 2;
      char_array_4[1] = ( (char_array_3[0] & 0x03) << 4 ) + ( (char_array_3[1] & 0xf0) >> 4 );
      char_array_4[2] = ( (char_array_3[1] & 0x0f) << 2 ) + ( (char_array_3[2] & 0xc0) >> 6 );
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (idx = 0; idx < 4; idx++) {
        encoded += base64_chars[char_array_4[idx]];
      }

      idx = 0;
    }
  }

  if (idx) {
    for (jdx = idx; jdx < 3; jdx++) {
      char_array_3[jdx] = '\0';
    }
     
    char_array_4[0] = ( char_array_3[0] & 0xfc ) >> 2;  
    char_array_4[1] = ( (char_array_3[0] & 0x03) << 4 ) + ( (char_array_3[1] & 0xf0) >> 4 );
    char_array_4[2] = ( (char_array_3[1] & 0x0f) << 2 ) + ( (char_array_3[2] & 0xc0) >> 6 );      

    for (jdx = 0; jdx < idx + 1; jdx++) {
      encoded += base64_chars[char_array_4[jdx]];
    }

    while (idx++ < 3) {
      encoded += '=';
    }
  }

  return encoded;
}

/* **************************************************************************
** Function: base64_decode
** Description: Decode a Base64 message to a std::string
** Source: https://github.com/ReneNyffenegger/cpp-base64/blob/master/base64.cpp#L87
** *************************************************************************/
String base64_decode(String const& message) {
  String decoded;
  int in_len = message.length();
  int idx = 0, jdx = 0, in_ = 0;
  unsigned char char_array_3[3], char_array_4[4];

  while ( in_len-- && ( message[in_] != '=' ) && is_base64( message[in_] ) ) {
    char_array_4[idx++] = message[in_]; in_++;
    
    if (idx == 4 ) {
      for (idx = 0; idx < 4; idx++) {
        char_array_4[idx] = base64_chars.indexOf( char_array_4[idx] );
      }

      char_array_3[0] = ( char_array_4[0] << 2 ) + ( (char_array_4[1] & 0x30) >> 4 );
      char_array_3[1] = ( (char_array_4[1] & 0x0f) << 4 ) + ( (char_array_4[2] & 0x3c) >> 2 );
      char_array_3[2] = ( (char_array_4[2] & 0x03) << 6 ) + char_array_4[3];

      for (idx = 0; idx < 3; idx++) {
        decoded += (char)char_array_3[idx];
      }

      idx = 0;
    }
  }

  if (idx) {
    for (jdx = 0; jdx < idx; jdx++) {
      char_array_4[jdx] = base64_chars.indexOf( char_array_4[jdx] );
    }

    char_array_3[0] = ( char_array_4[0] << 2 ) + ( (char_array_4[1] & 0x30) >> 4 ); 
    char_array_3[1] = ( (char_array_4[1] & 0x0f) << 4 ) + ( (char_array_4[2] & 0x3c) >> 2 );

    for (jdx = 0; jdx < idx - 1; jdx++) {
      decoded += (char)char_array_3[jdx];
    }
  }

  return decoded;
}


#include <Hash.h>
//#include <SHA256.h>
#include <BLAKE2s.h>
#include <AES.h>
#include <CTR.h>
#include <Crypto.h>


//String hmac(String mssg){
//  Serial.print("Hash size: ");
//  Serial.println(HASH_SIZE);
//  unsigned char result[HASH_SIZE];
//  unsigned char tmpres[HASH_SIZE];
//  int mlen = mssg.length();
//  char * mssgptr;
//  char noncebuf[12];
//  mssg.toCharArray(mssgptr, mlen);
//  itoa(NET_NONCE, noncebuf, 10);
//  byte tmpkey[32];
//  memset(result, 0x00, sizeof(result));
//  BLAKE2s *h;
//  //long keynum = this->key.toInt();
//  //keynum = keynum+nonce;
//  //String(keynum).getBytes(tmpkey, 32);
//
//  char key_chars[NET_KEY.length()];
//  NET_KEY.toCharArray(key_chars, sizeof(key_chars));
//
//
//  
//  byte key_bytes[33];
//  memset(key_bytes, '\0', 33);
//  NET_KEY.getBytes(key_bytes, 33);
//  
//    String cpystr1((char *)key_bytes);
//    String str1 = cpystr1;
//    Serial.print("Key bytes: ");
//    Serial.println(str1);
//    Serial.println("Key bytes ints: ");
//    for (int idx = 0; idx < 32; idx++) {
//        Serial.print((int)(str1.charAt(idx)));
//        Serial.print(" ");
//    }
//    Serial.println("");
//  Serial.println("RESET HMAC");
//  h->resetHMAC((void *)key_bytes, sizeof(key_bytes));
////  h->resetHMAC((void *)key_chars, 32);
//  Serial.println("UPDATE HMAC");
//  h->update(noncebuf, 12);
//  Serial.println("FINALIZE HMAC");
//  h->finalizeHMAC((const void *)key_chars, (size_t)12, (void *)tmpkey, (size_t)HASH_SIZE);
//
//  Serial.println("RESET HMAC");
//  h->resetHMAC(tmpkey, 32);
//  Serial.println("UPDATE HMAC");
//  h->update((const void *)mssgptr, (size_t)mlen);
//  Serial.println("FINALIZE HMAC");
//  h->finalizeHMAC((const void *)tmpkey, (size_t)mlen, (void *)result, (size_t)HASH_SIZE);
//  Serial.println(result[0]);
//  String trueres;
//  trueres = String((char *)result);
//  Serial.print("HMAC: ");
//  Serial.println(trueres);
//  return trueres;
//}

void hmac(Hash * h, byte * key, byte * result, byte * mssg) {
    h->resetHMAC(key, strlen((char *)key));
    h->update(mssg, strlen((char *)mssg));
    h->finalizeHMAC(key, strlen((char *)key), result, HASH_SIZE);
}

void hash(Hash * h, uint8_t * value, byte * mssg) {
    size_t inc = 1;
    size_t size = strlen((char *)mssg);
    size_t posn, len;

    h->reset();
    for (posn = 0; posn < size; posn += inc) {
        len = size - posn;
        if (len > inc) {
            len = inc;
        }
        h->update(mssg + posn, len);
    }
    h->finalize(value, HASH_SIZE);
}

void blake2s_test(byte * key, byte * mssg) {
    BLAKE2s blake2s;
    byte buffer[128];
    byte result[HASH_SIZE];

    memset(buffer, 0x00, sizeof(buffer));
    memset(result, 0x00, sizeof(result));

    hash(&blake2s, buffer, mssg);
    String cpystr1((char *)buffer);
    String str1 = cpystr1;
    Serial.print("Key bytes: ");
    Serial.println(str1);
    Serial.println("Key bytes ints: ");
    for (int idx = 0; idx < 32; idx++) {
        Serial.print((int)(str1.charAt(idx)));
        Serial.print(" ");
    }
    Serial.println("");

    // This one matches what is on the other devices
    hmac(&blake2s, key, result, mssg);
    String cpystr2((char *)result);
    String str2 = cpystr2;
    Serial.print("Key bytes: ");
    Serial.println(str2);
    Serial.println("Key bytes ints: ");
    for (int idx = 0; idx < 32; idx++) {
        Serial.print((int)(str2.charAt(idx)));
        Serial.print(" ");
    }
    Serial.println("");
}





void setup() {
    // Configure pins for Adafruit ATWINC1500 Feather
    WiFi.setPins(8,7,4,2);

    //Initialize serial and wait for port to open:
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    // Turn the LED on in order to indicate that the upload is complete
    pinMode(builtin_led, OUTPUT);

    // Check for the presence of the shield:
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("No WiFi shield");
        // Block forever
        while (true) {
          delay(1000);
        }
    }


    scan_available_networks();

    char ssid[] = "feather_m0_setup";
    start_access_point(ssid);

    // TESTING
    Serial.println("Ending AP");
    WiFi.end();
    state = AP_DETERMINED;
}



void encode_and_decode(String str) {
  String str_encoded = base64_encode(str, str.length());
  Serial.print("Encoded: ");
  Serial.println(str_encoded);

  String str_decoded = base64_decode(str_encoded);
  Serial.print("Decoded: ");
  Serial.println(str_decoded);
}



//void encrypt_and_decrypt(String plaintext) {
//    byte buffer[16];
//    CTR<AES256> aes256;
//  
//    byte key[32];
//    NET_KEY.getBytes(key, 32);
//    aes256.setKey(key, 32);
//    
//    byte iv[16];
//    memset(iv, '\0', 16);
//    String nonce(NET_NONCE);
//    Serial.print("NONCE: ");
//    Serial.println(nonce);
//    nonce.getBytes(iv, 16);
//    
//    String cpystr5((char *)iv);
//    String ciphertext5 = cpystr5;
//    Serial.print("IV: ");
//    Serial.println(ciphertext5);
//    
//    aes256.setIV(iv, 16);
//  
//    byte ptxt[16];
//    plaintext.getBytes(ptxt, 16);
//  
//    String cpystr2((char *)ptxt);
//    String ciphertext2 = cpystr2;
//    Serial.print("Plaintext: ");
//    Serial.println(ciphertext2);
//    
//    aes256.encrypt(buffer, ptxt, 16);
//  
//    String cpystr((char *)buffer);
//    String ciphertext = cpystr;
//    Serial.print("Ciphertext: ");
//    Serial.println(ciphertext);
//  
//    // Decrypt
//    byte buffer2[16];
//    aes256.setKey(key, 32);
//    aes256.setIV(iv, 16);
//    aes256.decrypt(buffer2, buffer, 16);
//  
//    
//    if (memcmp(ptxt, buffer2, 16) == 0) {
//        Serial.println("Matches");
//    } else {
//        Serial.println("Doesnt match");
//    }
//}

//void encrypt(String plaintext){
//    CTR<AES256> aes_ctr_256;
//    int ptxtlen = plaintext.length();
//    unsigned char ptxtptr[ptxtlen+1];
//    unsigned char iv[16];
//    itoa(this->nonce, (char *)iv, 10);
//    int ivlen = strlen((char *)iv);
//    plaintext.toCharArray((char *)ptxtptr, ptxtlen);
//    aes_ctr_256.setKey(key, 32);
//    aes_ctr_256.setIV(iv, ivlen);
//    aes_ctr_256.setCounterSize(4);
//    byte buffer[ptxtlen+1];//SEGFAULT? might need whole block of leeway. same w/ decrypt
//    aes_ctr_256.encrypt(buffer, ptxtptr, ptxtlen);
//
//    String cpystr((char *)buffer);
//    this->data = cpystr;
//    Serial.println(cpystr);
//    
//}



//String encrypt(String plaintext){
//    byte buffer[16];
//    CTR<AES256> aes256;
//
//    Serial.print("Aes key size: ");
//    Serial.println(aes256.keySize());
//    Serial.print("Aes IV size: ");
//    Serial.println(aes256.ivSize());
//  
//    byte key[32];
//    memset(key, '\0', 32);
//    NET_KEY.getBytes(key, 32);
//
//    String cpystr6((char *)key);
//    String ciphertext6 = cpystr6;
//    Serial.print("key: ");
//    Serial.println(ciphertext6);
//    Serial.println("Key ints: ");
//    for (int idx = 0; idx < 32; idx++) {
//        Serial.print((int)(ciphertext6.charAt(idx)));
//        Serial.print(" ");
//    }
//    Serial.println("");
//    
//    aes256.setKey(key, 32);
//    
//    
//    byte iv[16];
//    memset(iv, '\0', 16);
//    String nonce(NET_NONCE);
//    Serial.print("NONCE: ");
//    Serial.println(nonce);
//    nonce.getBytes(iv, 16);
//    
//    String cpystr5((char *)iv);
//    String ciphertext5 = cpystr5;
//    Serial.print("IV: ");
//    Serial.println(ciphertext5);
//    
//    Serial.println("IV ints: ");
//    for (int idx = 0; idx < 16; idx++) {
//        Serial.print((int)(ciphertext5.charAt(idx)));
//        Serial.print(" ");
//    }
//    Serial.println("");
//    
//    aes256.setIV(iv, 16);
//    
//    byte ptxt[16];
//    memset(ptxt, '\0', 16);
//    plaintext.getBytes(ptxt, 16);
//  
//    String cpystr2((char *)ptxt);
//    String ciphertext2 = cpystr2;
//    Serial.print("Plaintext: ");
//    Serial.println(ciphertext2);
//    
//    aes256.encrypt(buffer, ptxt, 16);
//  
//    String cpystr((char *)buffer);
//    String ciphertext = cpystr;
//    Serial.print("Ciphertext: ");
//    Serial.println(ciphertext);
//    Serial.println("Ciphertext ints: ");
//    for (int idx = 0; idx < ciphertext.length(); idx++) {
//        Serial.print((int)(ciphertext.charAt(idx)));
//        Serial.print(" ");
//    }
//    Serial.println("");
//
//    return ciphertext;
////    return base64_encode(ciphertext, ciphertext.length());
//}
//
//String decrypt(String ciphertext){
////    ciphertext = base64_decode(ciphertext);
//    Serial.print("Ciphertext decoded: ");
//    Serial.println(ciphertext);
//    CTR<AES256> aes256;
//  
//    byte key[32];
//    memset(key, '\0', 32);
//    NET_KEY.getBytes(key, 32);
//    aes256.setKey(key, 32);
//    
//    
//    byte iv[16];
//    memset(iv, '\0', 16);
//    String nonce(NET_NONCE);
//    Serial.print("NONCE: ");
//    Serial.println(nonce);
//    nonce.getBytes(iv, 16);
//    
//    String cpystr5((char *)iv);
//    String ciphertext5 = cpystr5;
//    Serial.print("IV: ");
//    Serial.println(ciphertext5);
//    
//    aes256.setIV(iv, 16);
//  
//    byte ctxt[16];
//    memset(ctxt, '\0', 16);
//    ciphertext.getBytes(ctxt, 16);
//  
//    // Decrypt
//    byte buffer2[16];
//    aes256.setKey(key, 32);
//    aes256.setIV(iv, 16);
//    aes256.decrypt(buffer2, ctxt, 16);
//
//    
//    String cpystr((char *)buffer2);
//    String plain = cpystr;
//    Serial.print("Plaintext: ");
//    Serial.println(plain);
//    return plain;
//}


byte ctxt[128];
// print out bytes
void aes256_enc(byte * key, byte * mssg, byte * iv) {

    String cpystr1((char *)key);
    String plain1 = cpystr1;
    Serial.print("Key: ");
    Serial.println(plain1);
    Serial.println("Key ints: ");
    for (int idx = 0; idx < 32; idx++) {
        Serial.print((int)(plain1.charAt(idx)));
        Serial.print(" ");
        Serial.print((int)(key[idx]));
        Serial.print(" ");
    }
    Serial.println("");

    String cpystr2((char *)mssg);
    String plain2 = cpystr2;
    Serial.print("Message: ");
    Serial.println(plain2);
    Serial.println("Message ints: ");
    for (int idx = 0; idx < 16; idx++) {
        Serial.print((int)(plain2.charAt(idx)));
        Serial.print(" ");
    }
    Serial.println("");

    String cpystr3((char *)iv);
    String plain3 = cpystr3;
    Serial.print("IV: ");
    Serial.println(plain3);
    Serial.println("IV ints: ");
    for (int idx = 0; idx < 16; idx++) {
        Serial.print((int)(plain3.charAt(idx)));
        Serial.print(" ");
    }
    Serial.println("");




  
    CTR<AES256> aes_ctr_256;
    memset(ctxt, 0x00, sizeof(ctxt));

    crypto_feed_watchdog();
    aes_ctr_256.setKey(key, 32);
    aes_ctr_256.setIV(iv, 16);
    aes_ctr_256.setCounterSize(4);
    aes_ctr_256.encrypt(ctxt, mssg, strlen((const char *)mssg));
    
    String cpystr((char *)ctxt);
    String plain = cpystr;
    Serial.print("Ciphertext: ");
    Serial.println(plain);

    
    
//    Serial.println("CTXT ints: ");
//    for (int idx = 0; idx < plain.length(); idx++) {
//        Serial.print((int)(plain.charAt(idx)));
//        Serial.print(" ");
//    }
//    Serial.println("");
}

void aes256_dec(byte * key, byte * iv, byte * buffer) {
    CTR<AES256> aes_ctr_256;
    memset(buffer, 0x00, sizeof(ctxt));

    crypto_feed_watchdog();
    aes_ctr_256.setKey(key, 32);
    aes_ctr_256.setIV(iv, 16);
    aes_ctr_256.setCounterSize(4);
    aes_ctr_256.decrypt(buffer, ctxt, strlen((const char *)ctxt));
    
    String cpystr((char *)buffer);
    String plain = cpystr;
    Serial.print("Plaintext: ");
    Serial.println(plain);
}


void loop() {
////    Serial.println("\nEncrypt and decrypt 1");
////    encrypt_and_decrypt("asdf");
//    Serial.println("\nBase64 encode and decode");
//    encode_and_decode("asdf");
//    Serial.println("\nEncrypt then decrypt");
////    String ctxt = "";
////    ctxt += (char)134;
////    ctxt += (char)145;
////    ctxt += (char)185;
////    ctxt += (char)9;
////    String ctxt = encrypt("3018");
////    Serial.print("Ciphertext encoded: ");
////    Serial.println(ctxt);
////    String dec = decrypt(ctxt);
////    Serial.print("Ciphertext Decrypted: ");
////    Serial.println(dec);
//    String message = "301812345";
////    byte mssg[] = "3018";
//    byte mssg[16];
//    memset(mssg, '\0', 16);
//    message.getBytes(mssg, 16);
////    byte key256[] = "01234567890123456789012345678901" + '\0';
//    byte key256[33];
//    memset(key256, '\0', 33);
//    NET_KEY.getBytes(key256, 33);
//    
////    byte iv[] = "998";
//    byte iv[16];
//    memset(iv, '\0', 16);
//    String nonce(NET_NONCE);
//    nonce.getBytes(iv, 16);
//
//    aes256_enc(key256, mssg, iv);
//    byte buffer[128];
//    aes256_dec(key256, iv, buffer);
//
//
//
//
//    // BLAKE2S
//    byte hmac_mssg[1024];
//    String hmac_mssg_str = "hmac this";
//    memset(hmac_mssg, '\0', 1024);
//    hmac_mssg_str.getBytes(hmac_mssg, 1024);
//    
//    byte key256_2[33];
//    memset(key256_2, '\0', 33);
//    NET_KEY.getBytes(key256_2, 33);
//    Serial.println("Blake2s test");
//    blake2s_test(key256_2, hmac_mssg);
//    
////    while (true);
    
    if (state == DETERMINE_NETWORK) {
        wifi_status = check_wifi_status(wifi_status);
      
        // Get a new client
        WiFiClient client = server.available();
      
        // If a client received
        if (client) {
            // Initialize a string to hold the data in the request from the client
            String currentLine = "";
      
            // Loop while the client is still connected
            while (client.connected()) {
              if (client.available()) {             // if there's bytes to read from the client,
              char c = client.read();             // read a byte, then
              Serial.write(c);                    // print it out the serial monitor
              if (c == '\n') {                    // if the byte is a newline character
      
                // if the current line is blank, you got two newline characters in a row.
                // that's the end of the client HTTP request, so send a response:
                if (currentLine.length() == 0) {
                  send_http_response(client);
                  // break out of the while loop:
                  
                  break;
                }
                else {      // if you got a newline, then clear currentLine:
                  currentLine = "";
                }
              }
              else if (c != '\r') {    // if you got anything else but a carriage return character,
                currentLine += c;      // add it to the end of the currentLine
              }
              else if (c == '\r') {
                  int action_page = currentLine.indexOf("action_page");
                  if (action_page > 0) {
                        currentLine = currentLine.substring(action_page);
                        Serial.print("Currentline outside: ");
                        Serial.println(currentLine);
                        if (currentLine.indexOf("ap_name=") > 0) {
                            int ap_num = currentLine.indexOf("ap_name=");
//                            currentLine = currentLine.substring(ap_num);
                            int ampersand_index = currentLine.indexOf("&host_ip");
                            String ap_num_str = currentLine.substring(ap_num+8, ampersand_index);
                            Serial.print("ap_num value is ");
                            Serial.print(ap_num_str);
                            Serial.println("");
                            access_point = AP_names[ap_num_str.toInt()];
                            Serial.print("Access point: ");
                            Serial.println(access_point);
                        }
                        Serial.print("Currentline outside2: ");
                        Serial.println(currentLine);
                        if (currentLine.indexOf("host_ip=") > 0) {
                            int host_ip = currentLine.indexOf("host_ip=");
//                            currentLine = currentLine.substring(host_ip);
                            int ampersand_index = currentLine.indexOf("&host_port");
                            String host_ip_str = currentLine.substring(host_ip+8, ampersand_index);
                            Serial.print("Host IP value is ");
                            Serial.println(host_ip_str);
//                            remote_server = host_ip_str;
                        }
                        Serial.print("Currentline outside3: ");
                        Serial.println(currentLine);
                        if (currentLine.indexOf("host_port=") > 0) {
                            int host_port = currentLine.indexOf("host_port=");
//                            currentLine = currentLine.substring(host_port);
                            int space_index = currentLine.indexOf("&device_id");
                            String host_port_str = currentLine.substring(host_port+10, space_index);
                            Serial.print("Host Port is ");
                            Serial.println(host_port_str);
                            remote_port = host_port_str.toInt();
                        }
                        Serial.print("Currentline outside4: ");
                        Serial.println(currentLine);
                        if (currentLine.indexOf("device_id=") > 0) {
                            int host_port = currentLine.indexOf("device_id=");
//                            currentLine = currentLine.substring(host_port);
                            int space_index = currentLine.indexOf(" HTTP");
                            String device_id_str = currentLine.substring(host_port+10, space_index);
                            Serial.print("Device ID is ");
                            Serial.println(device_id_str);
                            device_id = device_id_str;
                        }
                        client.stop();
                        WiFi.end();
                        state = AP_DETERMINED;
                    }
                }
            }
          }
          // close the connection:
          client.stop();
          Serial.println("client disconnected");
        }
    } else if (state == AP_DETERMINED) {
        // We have discovered an AP name so we want to connect to the WiFi network
        while (wifi_status != WL_CONNECTED) {
            Serial.print("Attempting to connect to WPA SSID: ");
            Serial.println(access_point);
            // Connect to WPA/WPA2 network:
              wifi_status = WiFi.begin("brypt1", "asdfasdf");
//            wifi_status = WiFi.begin(access_point);
        
            delay(1000);
        }
  
        // Print out information about the network
        Serial.print("Successfully connected to the network");
        printWiFiStatus();
  
        // Get the current time now that we (maybe) have internet
        Serial.print("Wifi Time: ");
        Serial.println(WiFi.getTime());
        state = AP_CONNECTED;
    } else if (state == AP_CONNECTED) {
        Serial.println("\nStarting connection to server...");
        // Try to connect to the TCP or StreamBridge socket
        if (server_connection.connect(remote_server, remote_port)) {
            Serial.println("Connected to server");
            state = SERVER_CONNECTED;
        } else {
//            delay(1000);
        }
    } else if (state == SERVER_CONNECTED && !contacted) {
        // As soon as we have connected to the socket, conduct initial_contact
        Serial.println("Setting up initial contact with coordinator");
        Serial.print("Connecting on port: ");
        Serial.println(remote_port);
        delay(100);

        String resp = "";

        while (resp.length() < 1  || (int)resp.charAt(0) != 6) {
            // Send ack
            String ack_msg = "\x06";
            Serial.println("Sending ACK");
            server_connection.print(ack_msg);
            server_connection.flush();
            delay(500);
    
            // Should get an ACK back
            resp = recv();
            Serial.print("Received: ");
            Serial.println((int)resp.charAt(0));
        }

        
        TechnologyType t = TCP_TYPE;
        String preferred_comm_tech = String((int)t);
        Serial.print("Going to send comm tech: ");
        Serial.println(preferred_comm_tech);
        delay(100);
        server_connection.print(preferred_comm_tech);
        server_connection.flush();
        delay(500);
        
        resp = recv();
        Serial.println("");
        Serial.print("Received: ");
        Serial.println(resp);
        Message port_message(resp);
        String coord_id = port_message.get_source_id();
        String req_port = port_message.get_data();
        Serial.print("Coord id: ");
        Serial.println(coord_id);
        Serial.print("Req port: ");
        Serial.println(req_port);

        Serial.println("Making info message");
        Serial.print("Port message nonce: ");
        Serial.println(port_message.get_nonce());
        Message info_message(device_id, coord_id, CONNECT_TYPE, 1, preferred_comm_tech, port_message.get_nonce());//use the nonce from the stored message
        delay(100);

        Serial.print("Sending info message: ");
        Serial.println(info_message.get_pack());
        server_connection.print(info_message.get_pack());
        server_connection.flush();
        delay(500);
        
        resp = recv();
        Serial.print("Received: ");
        Serial.println((int)resp.charAt(0));

        // Should shut down connection now
        server_connection.stop();

        // Connect to the new port
        Serial.print("Trying to connect to port: ");
        Serial.println(req_port.toInt());
        while (!server_connection.connect(remote_server, req_port.toInt())) {
            Serial.print("Trying to connect to port: ");
            Serial.println(req_port.toInt());
            delay(1000);
        }
        Serial.println("Connected to secondary port");
        contacted = 1;
    } else if (state == SERVER_CONNECTED && contacted) {
        String message = "";
        while (true) {
            cont_recv();
            delay(500);
        }
    }
}

String recv() {
    String message = "";
    while (server_connection.available()) {
        char c = server_connection.read();
//        Serial.write(c);
        message = message + c;
        // We have read the whole message
        if (c == (char)4) {
            return message;
//            handle_messaging(message);
        }
    }
    return message;
}

void cont_recv() {
    String message = "";
    while (server_connection.available()) {
        char c = '\0';
        c = server_connection.read();
        Serial.write(c);
        message = message + c;
        // We have read the whole message
        if (c == (char)4 || c == '=') {
            Serial.println("Found the end of the message");
            if (message.length() > 1) {
                message = message.substring(0, message.length()-1);
                handle_messaging(message);
                message = "";
                return;
            }
        }
    }
}




void handle_messaging(String message) {
    Message recvd_msg(message);
    
    Serial.println("");
    Serial.println("");
    Serial.print("Received message: ");
    Serial.println(recvd_msg.get_pack());
    Serial.print("Source ID: ");
    Serial.println(recvd_msg.get_source_id());
    Serial.print("Dest ID: ");
    Serial.println(recvd_msg.get_destination_id());
    Serial.print("Command: ");
    Serial.println(recvd_msg.get_command());
    Serial.print("Phase: ");
    Serial.println(recvd_msg.get_phase());
    Serial.print("Data: ");
    Serial.println(recvd_msg.get_data());

    switch(recvd_msg.get_command()) {
        case QUERY_TYPE: {
            switch(recvd_msg.get_phase()) {
                // Case 0 is the flood phase: Not handled by feathers because it has nothing to forward it on to
                case 1: {// Respond phase
                    // Check to see if the ID matches mine?
//                    String source_id = recvd_msg.get_destination_id();
                    String destination_id = recvd_msg.get_source_id();
                    String await_id = recvd_msg.get_await_id();
                    int phase = recvd_msg.get_phase();
                    if (await_id != "") {
                        destination_id = destination_id + ";" + await_id;
                    }
                    CommandType command = QUERY_TYPE;
                    unsigned int nonce = recvd_msg.get_nonce() + 1;
                    String data = "23.1"; // Bogus data

                    Message message(device_id, destination_id, command, phase, data, nonce);

                    String response = message.get_pack();
                    Serial.println("");
                    Serial.print("Responding with: ");
                    Serial.println(response);
                    Serial.print("Source ID: ");
                    Serial.println(recvd_msg.get_source_id());
                    Serial.print("Dest ID: ");
                    Serial.println(recvd_msg.get_destination_id());
                    Serial.print("Command: ");
                    Serial.println(recvd_msg.get_command());
                    Serial.print("Phase: ");
                    Serial.println(recvd_msg.get_phase());
                    Serial.print("Data: ");
                    Serial.println(recvd_msg.get_data());
                    server_connection.print(response);
                    server_connection.flush();
                    delay(100);
                    break;
                }
                // Case 2 is the aggregate phase: Not handled by feathers because they don't have anything to receive from
                case 3: {// Close phase
                    Serial.println();
                    Serial.println("RECEIVED CLOSE PHASE");
                    Serial.println();
                    break;
                }
            }
            break;
        }
        case INFORMATION_TYPE: {
            switch(recvd_msg.get_phase()) {
                // Case 0 is the flood phase: Not handled by feathers because it has nothing to forward it on to
                case 1: {// Respond phase
                    break;
                }
                // Case 2 is the aggregate phase: Not handled by feathers because they don't have anything to receive from
                case 3: {// Close phase
                    break;
                }
            }
            break;
        }
//        case HEARTBEAT_TYPE:
//            break;
    }
}

void printWiFiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
    // print where to go in a browser:
    Serial.print("To see this page in action, open a browser to http://");
    Serial.println(ip);

}

void listNetworks() {
    // Scan for nearby networks:
    Serial.println("** Scan Networks **");
    int numSsid = WiFi.scanNetworks();
    if (numSsid == -1) {
        Serial.println("Couldn't get a wifi connection");
        while (true);
    }

    // Print the list of networks seen:
    Serial.print("number of available networks:");
    Serial.println(numSsid);

    for (int i = 0; i < NETWORK_LIST_SIZE; i++) {
        memset(AP_names[i], '\0', AP_NAME_LEN);
    }
    // Print the network number and name for each network found:
    for (int thisNet = 0; thisNet < numSsid; thisNet++) {
        Serial.print(thisNet);
        Serial.print(") ");
        Serial.print(WiFi.SSID(thisNet));
        Serial.print("\tSignal: ");
        Serial.print(WiFi.RSSI(thisNet));
        Serial.print(" dBm");
        Serial.print("\tEncryption: ");
        printEncryptionType(WiFi.encryptionType(thisNet));
        Serial.flush();
        strncpy(AP_names[thisNet], WiFi.SSID(thisNet), (AP_NAME_LEN-1));
    }
}

void printEncryptionType(int thisType) {
    // read the encryption type and print out the name:
    switch (thisType) {
        case ENC_TYPE_WEP:
            Serial.println("WEP");
            break;
        case ENC_TYPE_TKIP:
            Serial.println("WPA");
            break;
        case ENC_TYPE_CCMP:
            Serial.println("WPA2");
            break;
        case ENC_TYPE_NONE:
            Serial.println("None");
            break;
        case ENC_TYPE_AUTO:
            Serial.println("Auto");
            break;
    }
}

void printMacAddress(byte mac[]) {
    for (int i = 5; i >= 0; i--) {
        if (mac[i] < 16) {
            Serial.print("0");
        }
        Serial.print(mac[i], HEX);
        if (i > 0) {
            Serial.print(":");
        }
    }
    Serial.println();
}

void scan_available_networks() {
    // Scan for existing networks:
    while (1) {
        Serial.println("Scanning available networks...");
        int brypt_found = 0;
        listNetworks();
        for (int i = 0; i < NETWORK_LIST_SIZE; i++) {
             Serial.print("Item is: ");
             Serial.println(AP_names[i]);
            if (strncmp("brypt", AP_names[i], 5) == 0) {
                // Serial.println("Found brypt thing");
                brypt_found = 1;
            }
        }
        if (brypt_found == 1) {
            break;
        }
//       TEMPORARY, DELETE LATER
//       break;
        // last_scan = millis();
        delay(10000);
    }
}

void start_access_point(char ssid[]) {
    // Print the network name (SSID);
    Serial.print("Creating access point named: ");
    Serial.println(ssid);

    // Create open network. Change this line if you want to create an WEP network:
    wifi_status = WiFi.beginAP(ssid);
    if (wifi_status != WL_AP_LISTENING) {
        Serial.println("Creating access point failed");
        // Block forever
        while (true);
    }

    // Start the web server on port 80
    server.begin();

    // Print the wifi status now that we are connected
    printWiFiStatus();
}

// add source id to web portal and add it to the parsing
void send_http_response(WiFiClient client) {
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  client.println("<body style=\"display: flex;    flex-direction: column;    justify-content: flex-start;    align-items: center;    background: #F4F7F8;    background: linear-gradient(to right, #E6E9F0, #F4F7F8);\">    <div class=\"box\" style=\"display: flex;    flex-direction: row;    justify-content: center;    align-items: center;    flex-wrap: nowrap;    height: 640px;    width: 420px;    background: #FBFBFB;    border-radius: 8px;    box-shadow: 0px 0px 20px rgba(30, 30, 30, 0.15);    box-sizing: border-box;    color: #4A5C62;    font-family: 'Source Sans Pro', sans-serif;    font-size: 14px;    font-weight: 300;    line-height: 1.15;    margin-top: 50px;\">  <form action=\"/action_page.php\">      <h1 style=\"position: relative;    width: 80%;    margin-bottom: 30px;    font-size: 2em;    color: #2D383C;    text-transform: uppercase;    text-align: center;    font-weight: 700;    font-family: 'Chivo', sans-serif;    margin-block-start: 0.83em;    margin-block-end: 0.83em;    box-sizing: border-box;    display: block;    line-height: 1.15;    margin: 0 auto;\">Available connections</h1>      <br>      <div class=\"listelems\" style=\"position: relative;    overflow-y: scroll;    height: 200px;    text-align: center;    width: 250px;    display: block;    position: relative;    display: block;    margin: 0 auto;\">");
  for (int i = 0; i < 10; i++) {
      if (strcmp(AP_names[i], "") != 0) {
          client.print("<div class=\"net-item\" style=\"padding-top: 5px;    padding-bottom: 5px;    transition: 0.3s ease-in-out all;\"><p>(");
          client.print(i);
          client.print(") ");
          client.print(AP_names[i]);
          client.println("</p></div>");
      }
  }
  // For some reason when adding styling to the inputs it causes issues
  client.println("</div>      <br>      <div class=\"entries\" style=\"text-align: center;    background: #FBFBFB;    border-radius: 8px;    box-sizing: border-box;    color: #4A5C62;    font-family: 'Source Sans Pro', sans-serif;    font-size: 14px;    font-weight: 300;    line-height: 1.15;\">      <input type=\"text\" name=\"ap_name\" placeholder=\"Access Point Number\" style=\"\">      <input type=\"text\" name=\"host_ip\" placeholder=\"Host IP Address\" style=\"\">      <input type=\"text\" name=\"host_port\" placeholder=\"Host Port Number\" style=\"\">      <input type=\"text\" name=\"device_id\" placeholder=\"Device ID Number\" style=\"\">      <input type=\"submit\" value=\"Submit\" style=\"position: relative;font-size: 1em;    font-weight: 700;    font-family: 'Chivo', sans-serif;    display: block;    position: relative;    top: 0px;    width: 250px;    height: 45px;    margin: 0 auto;margin-top: 10px;    font-size: 1.4em;    font-weight: 700;    text-transform: uppercase;    letter-spacing: 0.1em;    color: rgba(251, 251, 251, 0.6);    cursor: pointer;    -webkit-appearance: none;    background: rgb(26, 204, 149);    background: linear-gradient(to right top, rgb(26, 204, 148) 0%, rgb(67, 231, 179) 100%);    opacity: 0.85;    border: none;    box-shadow: 0px 0px 20px rgba(26, 204, 149, 0.5);    transition: 0.3s ease-in-out all;    border-radius: 4px;    text-decoration: none;\">      </div>  </form>    </div></body>");
  client.println();
}

int check_wifi_status(int wifi_status) {
    if (wifi_status != WiFi.status()) {
        // it has changed update the variable
        wifi_status = WiFi.status();
      
        if (wifi_status == WL_AP_CONNECTED) {
            byte remoteMac[6];
      
            // a device has connected to the AP
            Serial.print("Device connected to AP, MAC address: ");
            WiFi.APClientMacAddress(remoteMac);
            printMacAddress(remoteMac);
            // WiFi.end();
            // conn_stat = 1;
        } else {
            // a device has disconnected from the AP, and we are back in listening mode
            Serial.println("Device disconnected from AP");
        }
    }
    return wifi_status;
}

String handle_form_submission(WiFiClient client, String currentLine) {
    if (currentLine.indexOf("action_page") > 0) {
        int index = currentLine.indexOf("ap_name=");
        access_point = currentLine.substring(index+8);
        Serial.print("Access point is: ");
        Serial.println(access_point);
        int flag = 0;
        for (int i = 0; i < access_point.length(); i++) {
            if (access_point.charAt(i) == ' ') {
              Serial.println("Found space");
              access_point.setCharAt(i, '\0');
              break;
            }
        }
        Serial.print("Access point: ");
        Serial.println(access_point);
        client.stop();
        WiFi.end();
        state = AP_DETERMINED;
    }
}
