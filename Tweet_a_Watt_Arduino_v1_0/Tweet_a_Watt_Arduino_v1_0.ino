  #include <SoftwareSerial.h> //SoftwareSerial port for Xbee comm
  #include <SPI.h> //Comm to Ethernet stuff
  #include <Ethernet.h> //Ethernet stuff
  #include <avr/interrupt.h> //Convert floats to string to send to Cosm.com
      
  //I/O 8 is receive, and I/O 9 is transmit for Xbee comm
  uint8_t ssRX = 8;
  uint8_t ssTX = 9;
  
  SoftwareSerial xbee(ssRX, ssTX);
  
  #define APIKEY         "epqIULzyXJ-yn4ZlUq24Pzh4yr2SAKwvMThFM2gxendlQT0g" //Cosm API ID for my feed
  #define FEEDID         68173 //My Cosm feed ID
  #define USERAGENT      "Power Meter 1" //My Cosm feed name 
  
  #define MAX_PACKET_SIZE 110 //Leave a lot of room for arrays with entire packet 
  #define MAX_SAMPLE_SIZE 32  //Leave a lot of room for arrays with samples
  
  #define RMS_VOLTAGE 118   //!!!!Calibrate with the Volts displayed on Kill-a-Watt
  #define VREF_CALIBRATION 552 //!!!!Calibrate with the data in ampdata[i]
  
  #define CURRENT_NORM 15.5    //Converts ADC values to amps
  
  //Declare the variable for number of samples
  int total_samples;
  //Make array for the whole packet
  byte a[MAX_PACKET_SIZE];
  //Make arrays for the Volt and Amp ADC data
  //0 = Volts, 1 = Amps
  unsigned int ADC0[MAX_SAMPLE_SIZE];
  unsigned int ADC1[MAX_SAMPLE_SIZE];
  
  //Make arrays for the normalized 19 samples of Volts, Amps, and Watts data
  float ampdata[32];
  float voltdata[32];
  //Declare the variable for the final processed current and power data
  float avgamp;
  float avgwatt;
  
  //Define the MAC address of the Arduino
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

void setup() {
    //Start Hardware Serial
    Serial.begin(9600);
    //Start SoftwareSerial Xbee communications
    xbee.begin(9600);

  //Start the Ethernet connection, and if DHCP fails,
  if (Ethernet.begin(mac) == 0) {
      Serial.println("DHCP FAILED");
      //Use a fixed IP address, as determined above
      Ethernet.begin(mac, ip);
  }
  
  //Print the obtained IP address (for debugging)
  Serial.print("Client is at ");
  Serial.println(Ethernet.localIP());
}

void loop() {
    //Declare variable r
    int r;
    //Receive packet, and set r equal to the return of that function (1 if success)
    r = xbee_get_packet();
    //If packet was received, then put the data in nice arrays
    if(r) {
        r = xbee_interpret_packet(); 
    }
    //If the data put in the arrays sucessfully, then normalize the data in the ADC0 and ADC1 arrays
    if(r) {
        r = normalize_data();
    }    
    //If the data was normalized succesfully, then get average values for amps and watts ?????Maybe volts???
    if(r) {
        r = average_data_per_cycle();
    }
    //If the data averaged successfully, then push it to Cosm.com
    if(r){
      cosm_send();
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
           int t = 8;               
       
           //!!!!!!!!!!I may need to end up discarding the first sample!!!!!!!
           //Define and set s to 0; as long as it is less than the total number of samples (as defined above),
           for(int s = 0; s < total_samples; s++){
               
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

//         for (int f = 0; f < total_samples; f++) {
//               Serial.println(ADC1[f]);
//         }      
//         Serial.println();
          
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
       
    //Move the contents of ADC arrays to voltdata[] and ampdata[]; Print the contents(for debugging)
    for(int i = 0; i < total_samples; i++) {
        voltdata[i] = ADC0[i];
//        Serial.println(voltdata[i]);
    }
       
    for(int i = 0; i<total_samples; i++) {
        ampdata[i] = ADC1[i];
//        Serial.println(ampdata[i]);
    }
       
      
    //Normalize Volts
    
    int max_v = 0;
    int min_v = 1024;
    
    //Find the maximum and minimum voltage in a packet by comparing each sample to the previous max and min
    for(int i=0; i<total_samples; i++) {
        if (min_v > voltdata[i]) {
            min_v = voltdata[i];
        }
        if (max_v < voltdata[i]) {
            max_v = voltdata[i];
        }
    }
    
    //And print them(for debugging)
      
//  Serial.print("MAX:");
//  Serial.println(max_v);
//  Serial.println();
//  Serial.print("MIN:");
//  Serial.println(min_v);
//  Serial.println();
    
    //Declare and define average volts as the raw value that is the average of the minimum and maximum values = raw center between peaks 
    float averageV = (max_v + min_v) / 2;
    //Declare and define peak-to-peak voltage as the raw value that is the difference of the maximum and minimum values
    int vpp = max_v - min_v;
    //Declare and define the normalized peak-to-peak voltage as a known value, equal to the RMS voltage, times 2 square roots of 2
    float mainsvpp = RMS_VOLTAGE * (sqrt(2) * 2);
    //Print that value (for debugging)

//  Serial.print("mainsvpp: ");
//  Serial.println(mainsvpp);
//  Serial.println();


    //Calculate the normalized voltage by subtracing averageV, and then muliplying by mainsvpp, and dividing by vpp for each value in voltdata[]
    //and print (for debugging)
    for(int i = 0; i < total_samples; i++) {
        voltdata[i] -= averageV; 
//      Serial.println(voltdata[i]);

        voltdata[i] = (voltdata[i] * mainsvpp) / vpp;
//      Serial.println(voltdata[i]);  
//        Serial.println();  
    }
//  Serial.println();
//  Serial.println();


    //Normalize the amps data where, for each value in ampdata[] you subtract the hardcoded calibration data(the raw values coming over in ADC1[]
    //and then divide by the constant which converts the data to Amperes
    //Print (for debugging)    
    for(int i = 0; i < total_samples; i++) {
        ampdata[i] -= VREF_CALIBRATION;
//      Serial.println(ampdata[i]);
        ampdata[i] /= CURRENT_NORM;
//      Serial.println(ampdata[i]);  
    }
//  Serial.println(); 
     
              
    //This function now should return 2 arrays with reasonable data, 19 samples each
    //Print them (for debugging)
    
//  Serial.print("voltdata: ");
//  Serial.println();
//  for (int i = 0; i < total_samples; i++) {
//      Serial.println(voltdata[i]);
          
//  }
//  Serial.println();

//  Serial.print("ampdata: ");
//  Serial.println();
//  for (int i = 0; i < total_samples; i++) {
//      Serial.println(ampdata[i]);
//           
//  }
//      Serial.println();

    //Unless something happened that we didn't get here, always return 1
    return 1;

}



//Now we need to take the normalized arrays, and calculate one value for the amps and watts each cycle

boolean average_data_per_cycle(){
  
    //There are 16.6 samples per second
    float samples_per_second = 16.6;
    //Define avgamp as 0    
    avgamp = 0;
    //Calculate the actual amps/cycle by, 
    //Starting at 0, the average amps used per cycle are calculated by adding the absolute value of that iteration in ampdata[] to the previous value,
    for(int i = 0; i < samples_per_second; i++) {
        avgamp += abs(ampdata[i]);
    }    
    
    //and dividing the whole sum  by the samples per second
    avgamp /= samples_per_second;
    
    //To calculate the watts per cycle,
    //Use P=IV and multiple the calibrated RMS voltage by the averaged amps per cyle
    avgwatt = RMS_VOLTAGE * avgamp;
    
    //!!!Here watts are not really watts, they are VoltageAmps, which are slightly different; (look it up)
    
    //Print the final current readings, and power readings to serial (for debugging)
//    Serial.print("Final Current: ");
//    Serial.println(avgamp);
//    Serial.println();   
//    
//    Serial.print("Final VoltageAmps: ");
//    Serial.println(avgwatt);
//    Serial.println();
}


//Send the final data to Cosm.com

void cosm_send() {
    //Use dtostrf function to convert the float values to strings stored in C_data and P_data, and put them in a string with the labels Cosm expects
    char C_data[50];
    String dataString = "Current,";
    dtostrf(avgamp, 5, 2, C_data);
    dataString += String(C_data);
    char P_data[50];
    dataString += "\nPower,";
    dtostrf(avgwatt, 5, 2, P_data);
    dataString += String(P_data);

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
//        //If the connection failed, say so (for debugging)
//        Serial.println("connection failed");
//        Serial.println();
//        Serial.println("disconnecting.");
        client.stop();
    }
}
