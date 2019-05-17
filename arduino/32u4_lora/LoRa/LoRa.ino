/*
 * This code is borrowed from
 * https://github.com/sandeepmistry/arduino-LoRa
 * which itself is licensed under the MIT License.
 */

#include <SPI.h>
#include <LoRa.h>
#include "message.hpp"

void setup() {
  Serial.begin(9600);
//  while(!Serial);

  //Serial.println("LoRa started");

  LoRa.setPins(8,4,7);

  // Turn the LED on in order to indicate that the upload is complete
  pinMode(13, OUTPUT);
  
  if(!LoRa.begin(868100000)){
    //Serial.println("Starting LoRa failed!");
    while(1);
  }
  else{
    //Serial.println("Starting LoRa succeeded!");
  }

  LoRa.onReceive(onReceive);
  LoRa_rxMode();
}

void loop() {

}

void LoRa_rxMode(){
  LoRa.receive();
}

void LoRa_txMode(){
  LoRa.idle();
}

void onReceive(int packetSize){
  String received;
  received.reserve(160);
  received = "";
  String data PROGMEM = "68";
  
  while(LoRa.available()){
    received += (char)LoRa.read();
  }
  
  Message* incoming = new Message(received);
  Serial.print(F("Message received: "));
  Serial.println(incoming->get_data());

  if(incoming->get_source_id() == "1"){
    delete incoming;
  
    LoRa_txMode();
  
    delay(250);
    
    String response;
    response.reserve(160);
    Message outgoing("2", "1", QUERY_TYPE, 1, data, NET_NONCE);
    response = outgoing.get_pack();
    Serial.print(F("response = "));
    Serial.println(response);
    
    LoRa.beginPacket();
    LoRa.print(response);
    LoRa.endPacket();
    
    LoRa_rxMode();
  }
}
