  #include <SoftwareSerial.h>

  uint8_t ssRX = 8;
  uint8_t ssTX = 9;

  SoftwareSerial xbee(ssRX, ssTX);
  
  #define MAX_PACKET_SIZE 110
  #define MAX_SAMPLE_SIZE 32 
  int total_samples;
  //Make 2 new arrays for ADC pins 0, and 1
  unsigned int ADC0[MAX_SAMPLE_SIZE]; //Can I make an array length a variable????!!!!
  unsigned int ADC1[MAX_SAMPLE_SIZE];
  byte a[MAX_PACKET_SIZE];
  float ampdata[32];
  float wattdata[32];
  

void setup() {
    //Start Serial
    Serial.begin(9600);
    //Start SoftwareSerial Xbee communications
    xbee.begin(9600);
    }

void loop() {
  
          
          if(xbee_get_packet()){
            
         //   Serial.println("1:");
              if(xbee_interperet_packet()){
                   normalize_data();
                   average_data_per_cycle();
              }
              else {
              }  
        }
            else {
       //       Serial.println("0");
            } //xbee_interperet_packet();
          //} 
          
//          if(xbee_interperet_packet){
//               normalize_data(); //each loop should return a new set of calibrated values ready to be calculated to one value per second
//          }
          
          //if(xbee_interperet_packet){
              //!!!DO ALL THE GET 1 FLOAT AND INTERNET SERVER STUFF FUNCTION
          //}

}
  
  
  
  //TO FOLLOW: Functions to run in main loop
  
boolean xbee_get_packet(){ //!!!!!!!!!Should it be void or something else???
      //Define c as a variable to read 1 byte from SoftwareS 
    char c = xbee.read(); //!!!!!Is this declaration physically in the right place??????--What about all the other ints,
    //should they go further up? Down?
    
    //If the first byte is 0x7E then,
    if(c == 0x7E) {
        //Print SOH to SerialMonitor (for debugging)
 //       Serial.println("SOH");
 //       Serial.println();
        //Wait for next byte to come from xbee
        while(!xbee.available()); 
        //Define lengthMSB as the next byte, and read
        int lengthMSB = xbee.read();
        //Wait for next byte to come from xbee
        while(!xbee.available()); 
        //Define lengthLSB as the next byte, and read
        int lengthLSB = xbee.read();
        //And use them to calculate length of [???the rest of???] the incoming trasmission
        int length = (lengthLSB + (lengthMSB << 8)) + 1;
        //Print the length to SerialMonitor (for debugging)      
  //      Serial.print("Length of Packet:");
    //    Serial.println(length);
        Serial.println();
        
        //Define i as an integer equal to 0
        int i = 0;        
        //While i is less than that length,
        while (i<length) {
             //Wait for next byte to come from xbee
             while(!xbee.available());
             //Read that byte,
             c = xbee.read();
             //And save it to place i in array a
             a[i] = c;
             //then increment i, so the next byte is stored in the next location in the array,
             //and so on until all the bytes, in the calculated length, have been read and stored
             i++;
             }
             
             
        //Print each byte of the array (for debugging)
 //       Serial.print("Raw Packet:");
        for (i=0; i<length; i++) {
  //          Serial.println(a[i]);
        }
        return 1; //!!!!1 or true????!!!
   //     Serial.println();
     }
      

        //If 0x7E was not recieved print this (for debugging)
        else {
           // Serial.println("You've got a problem with your 0x7E byte!");

          return 0;
        }
}
    
boolean xbee_interperet_packet() {
        //If the first byte after the length is 0xFFFFFF83
        if (a[0] == 0x83) {
            //Print "SOH2" (for debugging)
 //           Serial.println("SOH2");
//            Serial.println();
            //and assign variables for the next two bytes
            int addrMSB = a[1];
            int addrLSB = a[2];
            //which are used to calculate the [???sender's????] address
            int address_16 = (addrMSB *256) + addrLSB;
            //Print the address (for debugging)
            //Serial.println(address_16);
            //Serial.println();
           //Asssign variables and print (for debugging) the rest of the header data
           //!!!!!!I still need to try to verify that these are correct!!!!!!!
           int RSSI = a[3];
           //Serial.println(RSSI);
           
           int address_broadcast = ((a[4] >> 1) & 0x01) == 1;
           //Serial.println(address_broadcast);
           
           int pan_broadcast = ((a[4] >> 2) & 0x01) == 1;
           //Serial.println(pan_broadcast);
           
           total_samples = a[5];
           //Serial.println(total_samples);
   
           int channel_indicator_high = a[6];
           //Serial.println(channel_indicator_high);
           
           int channel_indicator_low = a[7];
           //Serial.println(channel_indicator_low);
           
           //!!!!int local_checksum = //what goes here????
           //!!!!Put in checksum info

 
           //define a variable for how many active analog channels there are
           //and set it to 0
           int valid_analog = 0;
           int analog_channels = channel_indicator_high >> 1;

           //Define and set g equal to zero. Run this loop as long as g is less than the number of analog channels (6)
           //after each run increase g by 1 
           for(int g = 0; g<6; g++) { //should it be -6????????
               //If the channel indicator bit for the corresponding channel is set to 1,
               //!!!!!!Is g correct or does it need to be g-1, g+1?? The ADC channels are 0 through 5.
               if(analog_channels >> g & 1) { // ==1?????
                   //increase the variable for the number of active analog channels by 1
                   valid_analog++;
                   //At the end of the loop leaving "valid_analog" containing the number of active analog channels
               }
               //Serial.println(valid_analog);
               //Serial.println();
           }     
  
           //Set variable BPS to the number of bytes used per sample=the 2 digital bytes,
           //and the number of analog channels times their 2 bytes
           int bytes_per_sample = 2 * valid_analog; //!!!!!!!!!!!!!!!!!!!Is it not always constant?--data gets worse when = 12
           //Set t to the starting place in array a for the actual data after the header
           int t = 8;

           //In the future if more ADC sensors are added: 
           //int ADC2[19]; etc.                  
       
           //!!!!!!!!!!I may need to end up discarding the first sample!!!!!!!
           //Define and set s to 0; as long as it is less than the total number of samples (as defined above),
           for(int s = 0; s<total_samples; s++){
               
               //use the analog data to put an int at s in each array
               //s (and t) increment at each loop, thus assigning each sample to the next space in the array until it is full
               ADC0[s] = ((a[t] * 256) + a[t+1]); //is it | or + or other as per python code?
               ADC1[s] = ((a[t+2] * 256) + a[t+3]); // see xbee.py, arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1208712097
                    
                
               //add t (the starting place of the array) to the space taken up by all the bytes in one sample
               //so that for the next loop t can be the starting place of the next sample 
               t = t + bytes_per_sample; 
           }

//          //Print each byte of the array (for debugging)
            int f;
////          Serial.print("ADC0:");
////          for (f=0; f<total_samples; f++) {
////              Serial.println(ADC0[f]);
////          }
        //  Serial.println();
          //Serial.print("ADC1:");
          for (f=0; f<total_samples; f++) {
      //        Serial.println(ADC1[f]);
          }
          
        //  Serial.println();
          
          return 1;
      }
            
        
    

    
    //If 0xFFFFFF83 was not recieved print this (for debugging)
    else {
        
//      Serial.println("0x83 byte issue");
        return 0;     
  }
}

//Get actual data from received numbers function:

void normalize_data() { //!!!!should this be type void?????
       
        float voltdata[32];
        for(int i = 0; i<total_samples; i++) {
           voltdata[i] = ADC0[i];
       }
       
       for(int i = 0; i<total_samples; i++) {
           ampdata[i] = ADC1[i];
       }
       
      
    
    
    //Normalize Volts
    
    int max_v = 1024;
    int min_v = 0;
    
    
    for(int i=0; i<total_samples; i++) {
            if (min_v > voltdata[i]) {
                min_v = voltdata[i];
            }
            if (max_v < voltdata[i]) {
                max_v = voltdata[i];
            }
    }
    float averageV = (max_v + min_v) / 2;
    int vpp = max_v - min_v;
    int MAINSVPP = 170 * 2;
    

    for(int i=0; i<total_samples; i++) {
        voltdata[i] -= averageV; 
        //Serial.println(voltdata[i]);

        voltdata[i] = (voltdata[i] * 170 * 2) / vpp;
        //  Serial.println(voltdata[i]);  
         // Serial.println();  
}
   //  Serial.println();
     //Serial.println();


    int vrefcalibration = 523; //for sensor 1
    //In future add more (array where index is linked to xbee address, 1, 2, 3, etc.)
    float CURRENTNORM = 15.5;
 //   float ampdata[32];
    
    for(int i=0; i<total_samples; i++) {
        ampdata[i] -= vrefcalibration;
        //Serial.println(ampdata[i]);
        ampdata[i] /= CURRENTNORM;
        //Serial.println(ampdata[i]);  
  }
                
    //should return arrays with reasonable data, 19 samples
    Serial.print("Volts:");
    Serial.println();
    for (int i=0; i<total_samples; i++) {
        Serial.println(voltdata[i]);
           
    }
    Serial.println();
    Serial.print("Amperes:");
   Serial.println();
     for (int i=0; i<total_samples; i++) {
        Serial.println(ampdata[i]);
           
    }
    
    for(int i = 0; i<total_samples; i++){
        wattdata[i] = voltdata[i] * ampdata[i];
    }
    
}

void average_data_per_cycle(){
    float samples_per_second = 17.00; //16.6
    float avgamp = 0;
    for(int i = 0; i<samples_per_second; i++) {
        avgamp += abs(ampdata[i]);
    }    
    avgamp /= samples_per_second;
    Serial.print("Current:");
    Serial.println(avgamp);
    Serial.println();   
    
    float avgwatt = 0;
    for(int i = 0; i<samples_per_second; i++){
        avgwatt += abs(wattdata[i]);
    }
    
    avgwatt /= 17.00;

    Serial.print("VoltageAmps:");
    Serial.println(avgwatt);
    Serial.println();
}
