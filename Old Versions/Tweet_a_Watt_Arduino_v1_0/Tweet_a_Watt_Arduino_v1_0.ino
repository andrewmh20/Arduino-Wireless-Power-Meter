
#include <SoftwareSerial.h> //SoftwareSerial port for Xbee comm
#include <SPI.h> //Comm to Ethernet stuff
#include <Ethernet.h> //Ethernet stuff
#include <avr.h> //Convert floats to string to send to Cosm.com
#include <avr/wdt.h>
#define MOVING_AVERAGE_NUMBER 3
#define APIKEY         "epqIULzyXJ-yn4ZlUq24Pzh4yr2SAKwvMThFM2gxendlQT0g" //Cosm API ID for my feed
#define FEEDID         68173 //My Cosm feed ID
#define USERAGENT      "Power Meter 1" //My Cosm feed name 
#define MAX_PACKET_SIZE 110 //Leave a lot of room for arrays with entire packet 
#define MAX_SAMPLE_SIZE 32  //Leave a lot of room for arrays with samples
//Declare the variable for number of samples
int total_samples;
//Make array for the whole packet
byte a[MAX_PACKET_SIZE];
//Make arrays for the Volt and Amp ADC data
//0 = Volts, 1 = Amps
unsigned int ADC0[MAX_SAMPLE_SIZE];
unsigned int ADC1[MAX_SAMPLE_SIZE];
boolean first_time = true;  
float avgrmsV;
float avgrmsA;
//Make arrays for the normalized 19 samples of Volts, Amps, and Watts data
float ampdata[MAX_SAMPLE_SIZE];
float voltdata[MAX_SAMPLE_SIZE];
int ampref[MAX_SAMPLE_SIZE];
//Declare the variable for the final processed current and power data
float VA;
float rmsV;
float rmsA;
//Define  MAC address of the Arduino
float avgrmsV_array[MOVING_AVERAGE_NUMBER];
float avgrmsA_array[MOVING_AVERAGE_NUMBER];

//I/O 8 is receive, and I/O 9 is transmit for Xbee comm
uint8_t ssRX = A0;
uint8_t ssTX = 9;

SoftwareSerial xbee(ssRX, ssTX);

byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x0D, 0x27, 0x80 };
//Define the local IP address used by the Arduino if DHCP fails
IPAddress ip(10,10,5,110);

//Initialize the Ethernet library, designating the Arduino as a client
EthernetClient client;
//Declare "server" as the IP address of the server the Arduino will be accessing, in this case api.cosm.com
IPAddress server(216,52,233,121); 

//Last time a connection was made to the server, in milliseconds; setup to 0
unsigned long lastConnectionTime = 0;   
//Declare the last know state of the connection; setup as not connected    
boolean lastConnected = false;

int k = 0;
int e = 0;
void setup() {
  //Start Hardware Serial
  Serial.begin(9600);
  //Start SoftwareSerial Xbee communications
  xbee.begin(9600);



  //Start the Ethernet connection, and if DHCP fails,
  //  if (Ethernet.begin(mac) == 0) {
  //      Serial.println("DHCP FAILED");
  //      //Use a fixed IP address, as determined above
  Ethernet.begin(mac, ip);
  ////  }
  //  
  //Print the obtained IP address (for debugging)
  Serial.print("Client is at ");
  Serial.println(Ethernet.localIP());

  //tx 7
  //5v 5
  //gnd 3
  wdt_enable(WDTO_8S);  

}

void loop() {
  wdt_reset();
  //Declare variable r
  int r;
  //Receive packet, and set r equal to the return of that function (1 if success)
  r = xbee_get_packet();

  //If packet was received, then put the data in nice arrays
  if(r) {
    r = xbee_interpret_packet();
    Serial.println("Packet Interpreted");
    for(int i = 0; i<MOVING_AVERAGE_NUMBER; i++){

      //  Serial.println(avgrmsV_array[i]);

    }

  }
  //If the data put in the arrays sucessfully, then normalize the data in the ADC0 and ADC1 arrays
  //   if(k == 1){
  //     calibrate_amps();
  //   }
  if(r) {
    r = normalize_data();
    Serial.println("Data normalized");
  }    


  //If the data averaged successfully, then push it to Cosm.com
  if(r){
    cosm_send();
    //      Serial.println("Data Sent");
  }
  restart_ethernet();
  k++;

  e++;

}

void restart_ethernet(){
  if(e > 250) {
    if (Ethernet.begin(mac) == 0) {
      Serial.println("DHCP FAILED");
      //Use a fixed IP address, as determined above
      Ethernet.begin(mac, ip);
    }
  }
}

boolean xbee_get_packet(){
  //Wait until a byte becomes available from the xbee
  while(!xbee.available());
  //Define c as the first byte received from SoftwareSerial 
  char c = xbee.read();   
  //If the first byte is 0x7E then,
  if(c == 0x7E) {
    //Print SOH to SerialMonitor (for debugging)
    //      Serial.println("SOH");
    //      Serial.println();
    //Wait for next byte to come from xbee
    while(!xbee.available()); 
    //Define lengthMSB as the next byte, and read
    int lengthMSB = xbee.read();
    //Wait for next byte to come from xbee
    while(!xbee.available()); 
    //Define lengthLSB as the next byte, and read
    int lengthLSB = xbee.read();
    //And use them to calculate length of the rest of the incoming trasmission
    int length = (lengthLSB + (lengthMSB << 8)) + 1;
    //Print the length to SerialMonitor (for debugging)      
    //      Serial.print("Length of Packet: ");
    //      Serial.println(length);
    //      Serial.println();


    //Set i equal to 0, and when it is less than that length do the following,
    for(int i = 0; i < length; i++) {
      //Wait for next byte to come from xbee
      while(!xbee.available());
      //Read that byte,
      c = xbee.read();
      //And save it to place i in array a
      a[i] = c;
      //then increment i, so the next byte is stored in the next location in the array,
      //and so on, until all the bytes, in the calculated length, have been read and stored in a[]            
    }


    //Print each byte of the array (for debugging)
    //      Serial.print("Raw Packet: "); 
    //      for (int i=0; i < length; i++) {
    //          Serial.println(a[i], HEX);
    //      }
    //      Serial.println();

    //If 0x7E was found, and presumably this all executed well, then return 1
    Serial.println("Got packet");

    return 1;
  }


  //If 0x7E was not recieved print this (for debugging)
  else {
    //          Serial.println("You've got a problem with your 0x7E byte!");
    //and return 0
    return 0;
  }
}

boolean xbee_interpret_packet() {
  //If the first byte after the length is 0x83
  if (a[0] == 0x83) {
    //Print "SOH2" (for debugging)
    //          Serial.println("SOH2");
    //          Serial.println();

    //Assign variables for the next two bytes in the array
    int addrMSB = a[1];
    int addrLSB = a[2];
    //which are used to calculate the sender's address
    int address_16 = (addrMSB *256) + addrLSB;
    //Print the address (for debugging)
    //          Serial.println(address_16);
    //          Serial.println();

    //Asssign variables and print (for debugging) the rest of the header data           
    int RSSI = a[3];
    //         Serial.println(RSSI);

    int address_broadcast = ((a[4] >> 1) & 0x01) == 1;
    //         Serial.println(address_broadcast);

    int pan_broadcast = ((a[4] >> 2) & 0x01) == 1;
    //         Serial.println(pan_broadcast);

    total_samples = a[5];
    //         Serial.println(total_samples);

    int channel_indicator_high = a[6];
    //         Serial.println(channel_indicator_high);

    int channel_indicator_low = a[7];
    //         Serial.println(channel_indicator_low);

    //I still haven't put in checksum info

    //         Serial.println();


    //Define a variable for how many active analog channels there are
    //and set it to 0
    int valid_analog = 0;
    //Set variable for the bit indicating whether an analog channel is valid or not 
    int analog_channels = channel_indicator_high >> 1;

    //Define and set g equal to zero. Run this loop as long as g is less than the number of possible analog channels
    //after each run increase g by 1 
    for(int g = 0; g < 6; g++) {
      //If the channel indicator bit for the corresponding channel is set to 1,
      if(analog_channels >> g & 1) {
        //increase the variable for the number of active analog channels by 1
        valid_analog++;
        //At the end of the loop leaving "valid_analog" containing the number of active analog channels
      }

      //Print the number of valid analog channels (for debugging)
      //             Serial.println(valid_analog);
      //             Serial.println();

    }     

    //Set variable bytes per sample to the number of bytes used per sample = the number of valid analog channels * their 2 bytes
    int bytes_per_sample = 2 * valid_analog;

    //Set t to the starting place in array a for the actual data after the header
    int t = 8 + (bytes_per_sample * 2);               

    //!!!!!!!!!!I may need to end up discarding the first sample!!!!!!!
    //Define and set s to 0; as long as it is less than the total number of samples (as defined above),
    for(int s = 0; s < total_samples -2; s++){

      //use the analog data to put an int at s in each array
      //s (and t) increment at each loop, thus assigning each sample to the next space in the array until all the samples have been saved
      ADC0[s] = ((a[t] * 256) + a[t+1]); //Raw voltage data
      ADC1[s] = ((a[t+2] * 256) + a[t+3]); //Raw current data

      //add t (the starting place of the array) to the space taken up by all the bytes in one sample
      //so that for the next loop t can be the starting place of the next sample 
      t = t + bytes_per_sample; 
    }

    //Print each byte of the arrays (for debugging)

    //         Serial.print("ADC0: ");

    //         for(int f = 0; f < total_samples; f++) {
    //                Serial.println(ADC0[f]);
    //         }
    //         Serial.println();

    //         Serial.print("ADC1: ");

    for (int f = 0; f < total_samples-2; f++) {
      //               Serial.println(ADC1[f]);
    }      
    Serial.println();

    //If the 0x7E, and the 0x83 were found, and this presumably executed well, then return 1 
    return 1;
  }





  //If 0xFFFFFF83 was not recieved print this (for debugging)
  else { 
    //        Serial.println("0x83 byte issue");
    return 0;     
  }
}



boolean normalize_data() {


  if(!first_time) {
    for(int i = MOVING_AVERAGE_NUMBER-1; i>0; i--){
      avgrmsV_array[i] = avgrmsV_array[i-1];

    }
  }

  //Move the contents of ADC arrays to voltdata[] and ampdata[]; Print the contents(for debugging)


  int max_vd = 0;
  int min_vd = 1024;

  //Find the maximum and minimum voltage in a packet by comparing each sample to the previous max and min
  for(int i=0; i<total_samples-2; i++) {
    if (min_vd > ADC0[i]) {
      min_vd = ADC0[i];
    }
    if (max_vd < ADC0[i]) {
      max_vd = ADC0[i];
    }
  }
  float vzp = (max_vd + min_vd) / 2.0;

  for(int i = 0; i < total_samples-2; i++) {
    voltdata[i] = ADC0[i] - vzp; 
    voltdata[i] *= 0.59;  ///put into the sinosodial form based on the actual voltage
    //        Serial.println(voltdata[i]);
  }




  //Normalize Volts




  float max_v = -200.0;
  float min_v = 200.0;
  for(int i=0; i<total_samples-2; i++) {
    if (min_v > voltdata[i]) {
      min_v = voltdata[i];
    }
    if (max_v < voltdata[i]) {
      max_v = voltdata[i];
    }
  }

  //calc the peak volts, then dividee by root2 because its a sinosudial wave so that give RMS
  rmsV = ((max_v) * (sqrt(2.0)*.5)); 
  Serial.println(rmsV);
  if(!first_time){ 
    avgrmsV_array[0] = rmsV;
    avgrmsV = 0.0;
    for(int i = 0; i<MOVING_AVERAGE_NUMBER; i++){
      avgrmsV += avgrmsV_array[i];
      //  Serial.println(avgrmsV_array[i]);

    }
    avgrmsV /= MOVING_AVERAGE_NUMBER;
    Serial.println(avgrmsV);  
  }
  if(first_time){
    for(int i =0; i<MOVING_AVERAGE_NUMBER; i++){
      avgrmsV_array[i] = rmsV;
//      Serial.println(avgrmsV_array[i]);
    }
  }



  if(!first_time) {
    for(int i = MOVING_AVERAGE_NUMBER-1; i>0; i--){
      avgrmsA_array[i] = avgrmsA_array[i-1];

    }
  }

  float max_ad = 0;
  float min_ad = 1024;

  //Find the maximum and minimum voltage in a packet by comparing each sample to the previous max and min
  for(int i=0; i<total_samples-2; i++) {
    if (min_ad > ADC1[i]) {
      min_ad = ADC1[i];
    }
    if (max_ad < ADC1[i]) {
      max_ad = ADC1[i];
    }
  }

  float azp = (max_ad + min_ad) / 2;

  for(int i = 0; i<total_samples-2; i++) {
    ampdata[i] = ADC1[i] - azp;
    ampdata[i] /= 16.4;  
  }       


  rmsA = 0;
  for(int i = 0; i<total_samples-2; i++) {
    rmsA += sq(ampdata[i]);
  }
  rmsA /= total_samples-2;
  rmsA = sqrt(rmsA);
  Serial.println(rmsA);

  //Normalize the amps data where, for each value in ampdata[] you subtract the hardcoded calibration data(the raw values coming over in ADC1[]
    //and then divide by the constant which converts the data to Amperes
  //Print (for debugging)    
  if(!first_time){ 
    avgrmsA_array[0] = rmsA;
    avgrmsA = 0.0;
    for(int i = 0; i<MOVING_AVERAGE_NUMBER; i++){
      avgrmsA += avgrmsA_array[i];
      //  Serial.println(avgrmsA_array[i]);

    }
    avgrmsA /= MOVING_AVERAGE_NUMBER;
    Serial.println(avgrmsA);  
  }
  if(first_time){
    for(int i =0; i<MOVING_AVERAGE_NUMBER; i++){
      avgrmsA_array[i] = rmsA;
      //        Serial.println(avgrmsA_array[i]);
    }
  }



  VA = avgrmsV * avgrmsA;   //maybe subtract .03 first
  Serial.println(VA);

  if(!first_time) {
    for(int i = MOVING_AVERAGE_NUMBER-1; i>0; i--){
      avgwatt_array[i] = avgwatt_array[i-1];

    }
  }


  float watts = 0;

  for(int i = 0; i < total_samples-2; i++) {
    watts += (ampdata[i] * voltdata[i]);
  }

  watts /= (total_samples-2);
//  Serial.println(watts);

  if(!first_time){ 
    avgwatt_array[0] = watts;
    avgwatt = 0.0;
    for(int i = 0; i<MOVING_AVERAGE_NUMBER; i++){
      avgwatt += avgwatt_array[i];
      //  Serial.println(avgrmsA_array[i]);

    }
    avgwatt /= MOVING_AVERAGE_NUMBER;
    Serial.println(avgwatt);  
  }
  if(first_time){
    for(int i =0; i<MOVING_AVERAGE_NUMBER; i++){
      avgwatt_array[i] = watts;
      //        Serial.println(avgrmsA_array[i]);
    }
  }



  float PF = watts/VA;
  Serial.println(PF);


  if((rmsA < 100) && (VA < 10000)) {
    first_time = false; 
    return 1;
  }
  else {
    return 0;
  }



}

//Send the final data to Cosm.com

void cosm_send() {
  //Use dtostrf function to convert the float values to strings stored in C_data and P_data, and put them in a string with the labels Cosm expects
    char C_data[50];
  String dataString = "CurrentRMS,";
  dtostrf(rmsA, 5, 2, C_data);
  dataString += String(C_data);
  char V_data[50];
  dataString += "\nVoltsRMS,";
  dtostrf(rmsV, 5, 2, V_data);
  dataString += String(V_data);
  char VA_data[50];
  dataString += "\nVoltageAmps,";
  dtostrf(VA, 5, 2, VA_data);
  dataString += String(VA_data);
  //Print any incoming connection information to serial (for debugging, I never used it)
  //    if (client.available()) {
  //    char c = client.read();
  //    Serial.print(c);
  //    }

  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:                   !!!!!!WHY!?????
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("disconnecting...");
    client.stop();
  }

  //Actually send the data, using the function below
  sendData(dataString);

  //Store the state of the connection for next time through the loop main
  lastConnected = client.connected(); //!!!!!!!!!Doesn't this actually try to connect as well?????

}

//This is the function used above to actually send the data to Cosm.com:

//Have sendData called with the above string as the argument
void sendData(String thisData) {
  //If there's a successful connection on port 80 (HTTP),
  if (client.connect(server, 80)) {
    //Print "connecting" for debugging
    Serial.println("connecting...");
    //Send the HTTP PUT request:
    client.print("PUT /v2/feeds/");
    client.print(FEEDID);
    client.println(".csv HTTP/1.1");
    client.println("Host: api.cosm.com");
    client.print("X-ApiKey: ");
    client.println(APIKEY);
    client.print("User-Agent: ");
    client.println(USERAGENT);
    client.print("Content-Length: ");
    client.println(thisData.length());
    client.println("Content-Type: text/csv");
    client.println("Connection: close");
    client.println();
    //Actually send the string with the current and power readings
    client.println(thisData);
    //Print the string to serial (for debugging)
    //      Serial.println(thisData);
    client.stop(); //I don't have a clue why I need this, but it doesn't work without it :)
  } 
  else {
    //If the connection failed, say so (for debugging)
    Serial.println("connection failed");
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
}

