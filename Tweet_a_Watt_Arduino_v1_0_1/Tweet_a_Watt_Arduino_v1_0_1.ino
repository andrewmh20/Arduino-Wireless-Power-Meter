#include <SoftwareSerial.h> //SoftwareSerial port for Xbee comm
#include <SPI.h> //Comm to Ethernet stuff
#include <Ethernet.h> //Ethernet stuff
#include <avr.h> //Convert floats to string to send to Cosm.com
#include <avr/wdt.h> // Watch dog timer

#define APIKEY         "epqIULzyXJ-yn4ZlUq24Pzh4yr2SAKwvMThFM2gxendlQT0g" //Cosm API ID for my feed
#define FEEDID         68173 //My Cosm feed ID
#define USERAGENT      "Power Meter 1" //My Cosm feed name 
#define MAX_PACKET_SIZE 110 //Leave a lot of room for arrays with entire packet 
#define MAX_SAMPLE_SIZE 32  //Leave a lot of room for arrays with samples
#define MOVING_AVERAGE_NUMBER 3 // Number of samples to use for the moving average

int total_samples; //Declare the variable for number of samples
byte a[MAX_PACKET_SIZE]; //Make array for the whole packet
unsigned int ADC0[MAX_SAMPLE_SIZE]; //Make arrays for the Volt and Amp ADC data 0 = Volts, 1 = Amps
unsigned int ADC1[MAX_SAMPLE_SIZE];
boolean first_time = true; //Set to true for the first time the sketch main loop runs
float ampdata[MAX_SAMPLE_SIZE]; //Make arrays for the normalized 19 samples of Volts and Amps data
float voltdata[MAX_SAMPLE_SIZE];
float rmsV; //Declare the variables for the RMS volts and amps
float rmsA;
float VA; //Decare the variable for the apparent power
float avgrmsV_array[MOVING_AVERAGE_NUMBER]; //Declare arrays for the moving average samples
float avgrmsA_array[MOVING_AVERAGE_NUMBER];
float avgwatt_array[MOVING_AVERAGE_NUMBER];
float avgVA_array[MOVING_AVERAGE_NUMBER];
float avgrmsV; //Declare the variables for the moving averaged RMS values
float avgrmsA;
float avgwatt;
float avgVA;
/*
float avgPF_array[MOVING_AVERAGE_NUMBER];
float avgPF;
float PF;
*/
boolean lastConnected = false; //Declare the last know state of the ethernet connection; setup as not connected    
int e = 0; //Define counter  e
int k = 0;
//Define analog I/O 0 as receive, and analog I/O 1 as transmit for Xbee comm
uint8_t ssRX = A0;
uint8_t ssTX = A1;

SoftwareSerial xbee(ssRX, ssTX); //"Name" the software serial class xbee, and give it the I/O pins above for TX/RX

byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x00, 0xF5, 0x88 }; //Define the Arduino MAC address for networking
IPAddress ip(10,10,5,110); //Define the local IP address used by the Arduino if DHCP fails
EthernetClient client; //Initialize the Ethernet library, designating the Arduino as a client
IPAddress server(216,52,233,121); //Declare "server" as the IP address of the server the Arduino will be accessing, in this case api.cosm.com



void setup() {
  //Start Hardware Serial
  Serial.begin(9600);
  //Start SoftwareSerial Xbee communications
  xbee.begin(9600);

  //Start the Ethernet connection using DHCP, and if DHCP fails,
  if (Ethernet.begin(mac) == 0) {
    Serial.println("DHCP FAILED");
    //Use a fixed IP address, as determined above
    Ethernet.begin(mac, ip);
  }
  //Print the obtained IP address (for debugging)
  Serial.print(F("Client is at "));
  Serial.println(Ethernet.localIP());

  wdt_enable(WDTO_8S);  //Enable to watchdog timer so it resets the board after 8 seconds of no activity
}


void loop() {
  wdt_reset(); //Each time the main loop runs, reset the watchdog timer
  int r = xbee_get_packet(); //Set counter r = to the return of the function that gets the xbee packet
  //If packet was received, then put the data in nice arrays
  if(r) {
    r = xbee_interpret_packet(); //and set r to the return of that function, etc.
    Serial.println(F("Packet Interpreted"));
  }
  //Calculate the actual values from the data
  if(r) {
    r = normalize_data();
    Serial.println(F("Data normalized"));
  }    
  //If the data averaged successfully, then push it to Cosm.com
  if(r){
    Serial.println("R");
    if(k >= 15){
      Serial.println("K");
    cosm_send();
    Serial.println(F("Data Sent"));
    k = 0;
  }}

  
//  restart_ethernet(); //See if its time to restart ethernet yet  !!!!!!!!!!!!!!Idk if this actually works!!!!!!!!

  e++;
  k++;
}

void restart_ethernet(){
  //After 250 times through the loop, restart the internet connection
  if(e > 250) {
    if (Ethernet.begin(mac) == 0) {
      Serial.println("DHCP FAILED");
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
    Serial.println(F("Got packet"));

    return 1;
  }
  //If 0x7E was not recieved print this (for debugging)
  else {
    //      Serial.println("You've got a problem with your 0x7E byte!");
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
  //VOLTS:
  //If it is not the first time through the loop, then shift over the numbers in the moving average array, to make room for the new RMS value
  if(!first_time) {
    for(int i = MOVING_AVERAGE_NUMBER-1; i>0; i--){
      avgrmsV_array[i] = avgrmsV_array[i-1];
    }
  }
  int max_vd = 0;
  int min_vd = 1024;

  //Find the maximum and minimum digital voltage values in a packet by comparing each sample to the previous max and min
  for(int i=0; i<total_samples-2; i++) {
    if (min_vd > ADC0[i]) {
      min_vd = ADC0[i];
    }
    if (max_vd < ADC0[i]) {
      max_vd = ADC0[i];
    }
  }
  //Set the "zero point" (center of digital values) to the average of the maximum and minimum values
  float vzp = (max_vd + min_vd) / 2.0;
  //Transfer the ADC values to voltadata[] and calculate the actual voltage values
  for(int i = 0; i < total_samples-2; i++) {
    voltdata[i] = ADC0[i] - vzp; //Shift down the wave so that the center is at 0
    voltdata[i] *= 0.59;  //Multiply by a constant to convert the digital to actual voltage
    voltdata[i] -= 1;//?????????????????????//
    //        Serial.println(voltdata[i]);
  }

  float max_v = -200.0;
  float min_v = 200.0;
  //Calculate the max and min actual voltages
  for(int i=0; i<total_samples-2; i++) {
    if (min_v > voltdata[i]) {
      min_v = voltdata[i];
    }
    if (max_v < voltdata[i]) {
      max_v = voltdata[i];
    }
  }
  //Use the peak voltage to calculate RMS of sine wave
  rmsV = ((max_v) * (sqrt(2.0)*.5)); 
//  Serial.println(rmsV);
  
  //If it is not the first time through then put the new RMS in the moving average array,
  if(!first_time){ 
    avgrmsV_array[0] = rmsV;
    //and take the average of the last n samples
    avgrmsV = 0.0;
    for(int i = 0; i<MOVING_AVERAGE_NUMBER; i++){
      avgrmsV += avgrmsV_array[i];
      //  Serial.println(avgrmsV_array[i]);
    }
    avgrmsV /= MOVING_AVERAGE_NUMBER;
    Serial.println(avgrmsV);  
  }
  
  //If it is the first time through, populate the array with the first RMS value
  if(first_time){
    for(int i =0; i<MOVING_AVERAGE_NUMBER; i++){
      avgrmsV_array[i] = rmsV;
      //      Serial.println(avgrmsV_array[i]);
    }
  }


  //AMPS:
  
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

  float azp = (max_ad + min_ad) / 2.0;
  for(int i = 0; i<total_samples-2; i++) {
    ampdata[i] = ADC1[i] - azp;
    ampdata[i] /= 16.4;  //Constant
  }       

  rmsA = 0;
  //Calculate the current RMS knowing it could be any wave form, not just sine
  for(int i = 0; i<total_samples-2; i++) {
    rmsA += sq(ampdata[i]);
  }
  rmsA /= total_samples-2;
  rmsA = sqrt(rmsA);
  rmsA -= .03; //To get rid of consistent calculation error
//  Serial.println(rmsA);

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

  if(!first_time) {
    for(int i = MOVING_AVERAGE_NUMBER-1; i>0; i--){
      avgVA_array[i] = avgVA_array[i-1];

    }
  }

  //Calculate the apparent power
  VA = rmsV * rmsA;
  int VAint = int(VA);
  if(VAint < 1){
    VA = 0.0;
  }
    if(!first_time){ 
    avgVA_array[0] = VA;
    avgVA = 0.0;
    for(int i = 0; i<MOVING_AVERAGE_NUMBER; i++){
      avgVA += avgVA_array[i];
      //  Serial.println(avgrmsA_array[i]);
    }
    avgVA /= MOVING_AVERAGE_NUMBER;
    Serial.println(avgVA);  
  }
  
  if(first_time){
    for(int i =0; i<MOVING_AVERAGE_NUMBER; i++){
      avgVA_array[i] = VA;
      //        Serial.println(avgrmsA_array[i]);
    }
  }

  //Moving average for watts
  if(!first_time) {
    for(int i = MOVING_AVERAGE_NUMBER-1; i>0; i--){
      avgwatt_array[i] = avgwatt_array[i-1];
    }
  }

  float watts = 0.0;
  //Take the average of the instant watts = real power
  for(int i = 0; i < total_samples-2; i++) {
    watts += (ampdata[i] * voltdata[i]);
  }
  watts /= (total_samples-2);
  int wattsint = int(watts);
  if(wattsint < 1){
    watts = 0.0;
  }
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
    }
  }

/*

Power Factor calculations are NOT accurate enough for my purposes, but I have left some possible code in case I ever come back to it
//  if(!first_time) {
//    for(int i = MOVING_AVERAGE_NUMBER-1; i>0; i--){
//      avgPF_array[i] = avgPF_array[i-1];
//    }
//  }

  //Calculate the Power Factor = real power / apparent power
//  PF = (avgwatt*1000)/(avgVA*1000);
  
  PF = 1-((watts-VA)/VA);
  int PFint = int(PF);
if(PFint == 0){

    PF = 1.0;
}
    Serial.println(PF);

//  if(!first_time){ 
//    avgPF_array[0] = PF;
//    PF = 0.0;
//    for(int i = 0; i<MOVING_AVERAGE_NUMBER; i++){
//      avgPF += avgPF_array[i];
//      //  Serial.println(avgrmsA_array[i]);
//    }
//    avgPF /= MOVING_AVERAGE_NUMBER;
//    Serial.println(avgPF);  
//  }
//  
//  if(first_time){
//    for(int i =0; i<MOVING_AVERAGE_NUMBER; i++){
//      avgPF_array[i] = PF;
//    }
//  }
*/


  //If the data is good, and only then, change first time to true, and return 1 to move on with the sketch
  if((rmsA < 100) && (VA < 10000)) {
    first_time = false; 
    return 1;
  }
  else {
    return 0;
  }

}

//Send the final data to Cosm.com:
void cosm_send() {
  //Use dtostrf function to convert the float values to strings stored in C_data and P_data, and put them in a string with the labels Cosm expects
  char C_data[50];
  String dataString = "CurrentRMS,";
  dtostrf(avgrmsA, 5, 2, C_data);
  dataString += String(C_data);
  char V_data[50];
  dataString += "\nVoltsRMS,";
  dtostrf(avgrmsV, 5, 2, V_data);
  dataString += String(V_data);
  char VA_data[50];
  dataString += "\nVoltageAmps,";
  dtostrf(avgVA, 5, 2, VA_data);
  dataString += String(VA_data);
  char W_data[50];
  dataString += "\nWatts,";
  dtostrf(avgwatt, 5, 2, W_data);
  dataString += String(W_data);
  /*
  char PF_data[30];
  dataString += "\nPowerFactor,";
  dtostrf(PF, 5, 2, PF_data);
  dataString += String(PF_data);
  */
  //Print any incoming connection information to serial (for debugging, I never used it)
      if (client.available()) {
      char c = client.read();
      Serial.print(c);
      }

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

//Have sendData called with the above string as the argument
void sendData(String thisData) {
  //If there's a successful connection on port 80 (HTTP),
  if (client.connect(server, 80)) {
    //Print "connecting" for debugging
    Serial.println("connecting...");
    //Send the HTTP PUT request:
    client.print(F("PUT /v2/feeds/"));
    client.print(FEEDID);
    client.println(F(".csv HTTP/1.1"));
    client.println(F("Host: api.cosm.com"));
    client.print(F("X-ApiKey: "));
    client.println(APIKEY);
    client.print(F("User-Agent: "));
    client.println(USERAGENT);
    client.print(F("Content-Length: "));
    client.println(thisData.length());
    client.println(F("Content-Type: text/csv"));
    client.println(F("Connection: close"));
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


