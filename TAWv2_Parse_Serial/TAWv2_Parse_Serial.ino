#include <SoftwareSerial.h>

uint8_t ssRX = 8;
uint8_t ssTX =9;

SoftwareSerial xbee(ssRX, ssTX);


void setup() {
//Start Serial
  Serial.begin(9600);
//Start SoftwareSerial Xbee communications
xbee.begin(9600);
}

void loop()
{
//Define c as a variable to read 1 byte from SoftwareS 
  char c = xbee.read();
//If the first byte is 7E HEX then read the next 2 bytes
    if(c == 0x7E){
      
      int lengthMSB = xbee.read();
      int lengthLSB = xbee.read();
//And use them to calculate length of [???the rest of???] incoming trasmission
      int length = (lengthLSB + (lengthMSB << 8)) + 1;
//Print the length to Serial Monitor (For debugging)      
      Serial.println(length);
  int i = 0;
//Create an array with room for "length" number of values  
  int a[length];
//While i is less than that length, read 1 byte from Xbee
//and save it to the array
  while (i<length) {
  a[i] = c;
//then increment i, so the next byte is stored in the next location in the array
  i = i++;
}
//Print each byte of the array
  for (i=0; i < length; i++){
    Serial.println(a[i]);
                            }
 
 //***********Inside "if loop" put instructions for taking bytes from array
 //and putting them into a new int array,
 //Do I need to find 0x83 and treat as new packet, or can I just use array a[length]
 //???*******************
  }
 //If 7E was not found, then no packet was received   
    else {
      Serial.println("NO DATA RECEIVED");
}



}
