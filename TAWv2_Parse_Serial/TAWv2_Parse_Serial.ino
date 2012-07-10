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
  
    //Define c as a variable to read 1 byte from SoftwareS 
    char c = xbee.read(); //!!!!!Is this in the right place??????--What about all the other ints,
    //should they go further up? Down?
    
    //If the first byte is 0x7E then,
    if(c == 0x7E) {
        //Print SOH to SerialMonitor (for debugging)
        Serial.println("SOH");
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
        Serial.println(length);
        
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
             
             
        //Print each byte of the array (for debuggig)
        for (i=0; i<length; i++) {
        Serial.println(a[i]);
        }
        
        //If the first byte after the length is 0xFFFFFF83
        if (a[0] == 0xFFFFFF83) {
            //Print "SOH2" (for debugging)
            Serial.println("SOH2");
            //and assign variables for the next two bytes
            int addrMSB = a[1];
            int addrLSB = a[2];
            //which are used to calculate the [???sender's????] address
            int address_16 = (addrMSB << 8) + addrLSB;
            //Print the address (for debugging)
            Serial.println(address_16);
   
           //Asssign variables and print (for debugging) the rest of the header data
           //!!!!!!I still need to try to verify that these are correct!!!!!!!
           int RSSI = a[3];
           Serial.println(RSSI);
           
           int address_broadcast = ((a[4] >> 1) & 0x01) == 1;
           Serial.println(address_broadcast);
           
           int pan_broadcast = ((a[4] >> 2) & 0x01) == 1;
           Serial.println(pan_broadcast);
           
           int total_samples = a[5];
           Serial.println(total_samples);
   
           int channel_indicator_high = a[6];
           Serial.println(channel_indicator_high);
           
           int channel_indicator_low = a[7];
           Serial.println(channel_indicator_low);
           
           //!!!!int local_checksum = //what goes here????
           //!!!!Put in checksum info

 
           //define a variable for how many active analog channels there are
           //and set it to 0
           int channel_count = 0;
            
           //Define and set g equal to zero. Run this loop as long as g is less than the number of analog channels (6)
           //after each run increase g by 1 
           for(int g = 0; g<6; g++) {
               //If the channel indicator bit for the corresponding channel is set to 1,
               //!!!!!!Is g correct or does it need to be g-1, g+1?? The ADC channels are 0 through 5.
               if(channel_indicator_high >> g & 1) {
                   //increase the variable for the number of active analog channels by 1
                   channel_count++;
                   //At the end of the loop leaving "channel_count" containing the number of active analog channels
               }
           }
  
           //Set variable BPS to the number of bytes used per sample=the 2 digital bytes,
           //and the number of analog channels times their 2 bytes
           int bytes_per_sample = 2 + (channel_count*2);
           //Set t to the starting place in array a for the actual data after the header
           int t = 8;
           //Make 2 new arrays for ADC pins 0, and 1
           int ADC0[19];
           int ADC1[19];
           
           //In the future id more ADC sensors are added: 
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
            }
        }
        
        //If 0x7E was not recieved print this (for debugging)
        else {
            Serial.println("You've got a problem with your 0xFFFFFF83 byte!");
        }
    }
    
    //If 0xFFFFFF83 was not recieved print this (for debugging)
    else {
        Serial.println("NO DATA RECEIVED (or just bad data)");
    }
}
