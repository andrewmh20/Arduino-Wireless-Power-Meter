  #include <SoftwareSerial.h>

  uint8_t ssRX = 8;
  uint8_t ssTX = 9;

  SoftwareSerial xbee(ssRX, ssTX);

int total_samples;
           //Make 2 new arrays for ADC pins 0, and 1
           int ADC0[19]; //Can I make an array length a variable????!!!!
           int ADC1[19];
           

void setup() {
    //Start Serial
    Serial.begin(9600);
    //Start SoftwareSerial Xbee communications
    xbee.begin(9600);
    }

void loop() {
  
    //Define c as a variable to read 1 byte from SoftwareS 
    char c = xbee.read(); //!!!!!Is this declaration physically in the right place??????--What about all the other ints,
    //should they go further up? Down?
    
    //If the first byte is 0x7E then,
    if(c == 0x7E) {
        //Print SOH to SerialMonitor (for debugging)
        //Serial.println("SOH");
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
        //Serial.println(length);
        
        //Define i as an integer equal to 0
        int i = 0;
        //Create an array with room for "length" number of values  
        int a[length];
        
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
        for (i=0; i<length; i++) {
        //Serial.println(a[i]);
        }
        
        //If the first byte after the length is 0xFFFFFF83
        if (a[0] == 0xFFFFFF83) {
            //Print "SOH2" (for debugging)
            //Serial.println("SOH2");
            //and assign variables for the next two bytes
            int addrMSB = a[1];
            int addrLSB = a[2];
            //which are used to calculate the [???sender's????] address
            int address_16 = (addrMSB << 8) + addrLSB;
            //Print the address (for debugging)
            //Serial.println(address_16);
   
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
           int analog_channels = channel_indicator_high>>1;

           //Define and set g equal to zero. Run this loop as long as g is less than the number of analog channels (6)
           //after each run increase g by 1 
           for(int g = 0; g<(6); g++) { //should it be -6????????
               //If the channel indicator bit for the corresponding channel is set to 1,
               //!!!!!!Is g correct or does it need to be g-1, g+1?? The ADC channels are 0 through 5.
               if(analog_channels >> g & 1) { // ==1?????
                   //increase the variable for the number of active analog channels by 1
                   valid_analog++;
                   //At the end of the loop leaving "channel_count" containing the number of active analog channels
               }
           }     
  
           //Set variable BPS to the number of bytes used per sample=the 2 digital bytes,
           //and the number of analog channels times their 2 bytes
           int bytes_per_sample = 14; //Is it not always constant?--data gets worse when = 12
           //Set t to the starting place in array a for the actual data after the header
           int t = 8;

           //In the future if more ADC sensors are added: 
           //int ADC2[19]; etc.
           
           //!!!!!I am not processing digital data!!!!!!
           
           
           //!!!!!!!!!!I may need to end up discarding the first sample!!!!!!!
           //Define and set s to 0; as long as it is less than the total number of samples (as defined above),
           for(int s = 0; s<total_samples; s++){
               
               //skip over the digital data and use the analog data to put an int at s in each array
               //s (and t) increment at each loop, thus assigning each sample to the next space in the array until it is full
               ADC0[s] = ((a[t+2] << 8) + a[t+3]);
               ADC1[s] = ((a[t+4] << 8) + a[t+5]);
                
               //add t (the starting place of the array) to the space taken up by all the bytes in one sample
               //so that for the next loop t can be the starting place of the next sample 
               t = t + bytes_per_sample; 
               Serial.println(ADC1[s]);
            }

          //Print each byte of the array (for debugging)
          int f;
          for (f=0; f<total_samples; f++) {
              //Serial.println(ADC0[f]);
              //Serial.println(ADC1[f]);
          }
          Serial.println();
          
          
       }
            
       
        //If 0x7E was not recieved print this (for debugging)
        else {
            //Serial.println("You've got a problem with your 0xFFFFFF83 byte!");
        }
    }
    
    //If 0xFFFFFF83 was not recieved print this (for debugging)
    else {
        //Serial.println("NO DATA RECEIVED (or just bad data)");
    }
}

          //Average VOLTS function:

void normalize_data() { //!!!!should this be type void?????
    int max_v = 1024;
    int min_v = 0;
    int i;
    
    for(i=0; i<total_samples; i++) {
            if (min_v > ADC0[i]) {
                min_v = ADC0[i];
            }
            if (max_v < ADC0[i]) {
                max_v = ADC0[i];
            }
    }
    float averageV = (max_v + min_v) / 2;
    int vpp = max_v - min_v;
    int MAINSVPP = 170 * 2;
    
    for(i=0; i<total_samples; i++) {
        ADC0[i] = ADC0[i] - averageV; 
        ADC0[i] = (ADC0[i] * MAINSVPP) / vpp;
    }
    
    int vrefcalibration = 520; //for sensor 1
    //In future add more (array where index is linked to xbee address, 1, 2, 3, etc.)
    float CURRENTNORM = 15.5;
    for(i=0; i<total_samples; i++) {
        ADC1[i] = ADC1[i] - vrefcalibration;
        ADC1[i] = ADC1[i] / CURRENTNORM;
    }
                

    for (i=0; i<total_samples; i++) {
        Serial.println(ADC0[i]);
        Serial.println(ADC1[i]);    
    }
}