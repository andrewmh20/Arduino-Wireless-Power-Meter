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
    if(xbee.read() == 0x7E){
Serial.println("DATA ACQUIRED");
    }
else {
  Serial.println("Another yummy byte received");
}
}
      
