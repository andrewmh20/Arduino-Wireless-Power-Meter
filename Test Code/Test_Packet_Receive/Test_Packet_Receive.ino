#include <SoftwareSerial.h>

uint8_t ssRX = 8;
uint8_t ssTX =9;

SoftwareSerial xbee(ssRX, ssTX);


void setup() {
Serial.begin(9600);
xbee.begin(9600);
}

void loop()
{
  
  char c = xbee.read();
    if(c == 0x7E){
Serial.println("SOH");
    }
else {
  Serial.println(c, HEX);
}
}
      
