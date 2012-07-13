  #include <SoftwareSerial.h>
  #include <SD.h>
  
  //I/O 8 is receive, and I/O 9 is transmit
  uint8_t ssRX = 8;
  uint8_t ssTX = 9;
  
  
  SoftwareSerial xbee(ssRX, ssTX);
  
  #define MAX_PACKET_SIZE 110 //Leave a lot of room for arrays with packet 
  #define MAX_SAMPLE_SIZE 32  //Leave a lot of room for arrays with samples
  #define RMS_VOLTAGE 123   //Calibrate with the Volts displayed on Kill-a-Watt
  #define VREF_CALIBRATION 552 //Calibrate with the data in ampdata[i]
  #define CURRENT_NORM 15.5    //Convert ADC values  to amps
  
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
  float avgamp;
  float avgwatt;
  File taw;

void setup() {
    //Start Hardware Serial
    Serial.begin(9600);
    //Start SoftwareSerial Xbee communications
    xbee.begin(9600);
    //Start SD service on pin 4
    SD.begin(4);
//    //Remove any previous TAW.txt file
//    taw.close();
//    SD.remove("TAW.txt");
}

void loop() {
    int r;
    //Receive packet
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
    if(r) {
      sd_store();
    }
    
//Insert web server stuff
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
        

        //Set i equalt to 0, and when it is less than that length do the following,
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
    
    //Print the final current readings, and power readings to serial
    Serial.print("Final Current: ");
    Serial.println(avgamp);
    Serial.println();   
    
    Serial.print("Final VoltageAmps: ");
    Serial.println(avgwatt);
    Serial.println();
}

//Interface with the SD card
void sd_store(){
    //Open (first time create???) TAW.txt and ????set it to be able to write to it???
    taw = SD.open("TAW.txt", FILE_WRITE);
    //If the file was succesfully opened, 
    if(taw) {
        //Store the current and VA readings in TAW.txt
        taw.print("Final Current: ");
        taw.println(avgamp);
        taw.println();   
    
        taw.print("Final VoltageAmps: ");
        taw.println(avgwatt);
        taw.println();
        
        //Close the TAW file  
        taw.close();
    }
    
}
