  #include <SoftwareSerial.h>

  uint8_t ssRX = 8;
  uint8_t ssTX = 9;

  SoftwareSerial xbee(ssRX, ssTX);
  
  

void setup() {
    //Start Serial
    Serial.begin(9600);
    //Start SoftwareSerial Xbee communications
    xbee.begin(9600);
    }

void loop() {
  
  while(!xbee.available());
  Serial.println(xbee.read(), HEX);
}
