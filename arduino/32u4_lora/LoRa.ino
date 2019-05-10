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
  if(runEvery(5000)){
    //Serial.print("Sending packet: ");
    String data = "65.2";
    LoRa_sendMessage(data);
    Serial.println(data + " " + NET_NONCE);
  }
}

void LoRa_rxMode(){
  //LoRa.enableInvertIQ();
  LoRa.receive();
}

void LoRa_txMode(){
  LoRa.idle();
  //LoRa.disableInvertIQ();
}

void LoRa_sendMessage(String data){
  LoRa_txMode();
  Message message("2", "1", QUERY_TYPE, 1, data, NET_NONCE);
  String response = message.get_pack();
  Serial.println("In send after get_pack");
  LoRa.beginPacket();
  LoRa.print(response);
  LoRa.endPacket();
  LoRa_rxMode();
}

void onReceive(int packetSize){
  String received;
  received.reserve(128);
  received = "";
  
  while(LoRa.available()){
    received += (char)LoRa.read();
  }

  Message message(received);
  
  Serial.print("Message Received: ");
  Serial.println(message.get_data());
}

boolean runEvery(unsigned long interval){
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval){
    previousMillis = currentMillis;
    return true;
  }
  return false;
}
