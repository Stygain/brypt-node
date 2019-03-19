#include <SoftwareSerial.h>
#include <Crypto.h>
#include <Hash.h>
#include <BLAKE2s.h>

#define HASH_SIZE 32

void hmac(Hash *h, byte *key, byte *result, byte *mssg){
  h->resetHMAC(key, strlen((char*)key));
  h->update(mssg, strlen((char*)mssg));
  h->finalizeHMAC(key, strlen((char*)key), result, HASH_SIZE);
}

void hash(Hash *h, uint8_t *value, byte *mssg){
    size_t inc = 1;
    size_t size = strlen((char*)mssg);
    size_t posn, len;

    h->reset();
    for (posn = 0; posn < size; posn += inc) {
        len = size - posn;
        if (len > inc)
            len = inc;
        h->update(mssg + posn, len);
    }
    h->finalize(value, HASH_SIZE);
}

void blake2s_test(byte *key, byte *mssg){
  BLAKE2s blake2s; 
  byte buffer[128];
  byte result[HASH_SIZE];

  memset(buffer, 0x00, sizeof(buffer));
  memset(result, 0x00, sizeof(result));
   
  hash(&blake2s, buffer, mssg);
  hmac(&blake2s, key, result, mssg); 
}

SoftwareSerial mySerial(10, 11); //rx, tx

void setup() {
  // put your setup code here, to run once:

 /* Serial.begin(57600);
  while(!Serial){
    ;
  }*/
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long long t_start = 0;
  unsigned long long t_end = 0;
  unsigned long duration;
  byte mssg[] = "The quick brown fox jumps over the lazy dog";
  byte key256[] = "01234567890123456789012345678901";

  t_start = millis();
  for(unsigned long i = 0; i < 5000/*30000000*/; i++){
    blake2s_test(key256, mssg);
  }
  
  t_end = millis();

  duration = (unsigned long)(t_end - t_start);
  Serial.println("\nDone");
  Serial.print((unsigned long)t_end, DEC);
  Serial.println("\nStart");
  Serial.print((unsigned long)t_start, DEC);
  Serial.println("\nDuration:");
  Serial.print((unsigned long)duration, DEC);
  Serial.println("\n");
  
  /*while(1){
    ;
  }*/
}
