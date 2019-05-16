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
  while(!Serial);

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
//  if(runEvery(  5000)){
//    sendMessage();
//  }
}

//void sendMessage(){
//  String data PROGMEM = "68";
//  LoRa_txMode();
//  String response;
//  response.reserve(160);
//  Message outgoing("2", "1", QUERY_TYPE, 1, data, NET_NONCE);
//  response = outgoing.get_pack();
//  Serial.println("response = " + response);
//  LoRa.beginPacket();
//  LoRa.print(response);
//  LoRa.endPacket();
//  LoRa_rxMode();
//}

void LoRa_rxMode(){
  //LoRa.enableInvertIQ();
  LoRa.receive();
}

void LoRa_txMode(){
  LoRa.idle();
  //LoRa.disableInvertIQ();
}

void onReceive(int packetSize){
  String received;
  received.reserve(160);
  received = "";
  String data PROGMEM = "68";
  
  while(LoRa.available()){
    received += (char)LoRa.read();
  }

  Message incoming(received);
  Serial.print(F("Message received: "));
  Serial.println(incoming.get_data());

//  delay(500);
//
  LoRa_txMode();
  String response;
  response.reserve(160);
  Message outgoing("2", "1", QUERY_TYPE, 1, data, incoming.get_nonce());
  response = outgoing.get_pack();
  Serial.print(F("response = "));
//  Serial.println(response);
//  LoRa.beginPacket();
//  LoRa.print(response);
//  LoRa.endPacket();
  LoRa_rxMode();
}

//boolean runEvery(unsigned long interval)
//{
//  static unsigned long previousMillis = 0;
//  unsigned long currentMillis = millis();
//  if (currentMillis - previousMillis >= interval)
//  {
//    previousMillis = currentMillis;
//    return true;
//  }
//  return false;
//}
