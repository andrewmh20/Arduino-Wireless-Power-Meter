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

  int N = 68;
  int a[N];
  
  int i = 0;
  char c;
  
  while (i<N) {
   
    c = xbee.read();
    a[i] = c;
    i = i++;
  }
 
 
for (i=0; i < N; i++){
  Serial.println(a[i]);
  
}

  }


