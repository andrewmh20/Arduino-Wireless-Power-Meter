Project by Andrew Merczynski-Hait, andrewmh20@nyc.rr.com, 
Based off of the project by Lady Ada, http://www.ladyada.net/make/tweetawatt/index.html



This code is licensed under ....... Please see the attached license file.


Harware Instructions:

I had an extra Xbee adapter board on hand, so for one used as the receiver I soldered the headers on it as Lady Ada suggests to use the FTDI cable for communication directly with the computer. However, for the second one, and the one I used in the end for the receiver I soldered vertical headers, and only on the pins I needed in order to facilitate better connection with the Arduino.

Instructions for installation:

I diverged from the instructions for Lady Ada's Tweet-a-Watt when I used analog input 0 and 1 on the transmitter Xbee, instead of analog input 0 and 4. Make sure to change the corresponding define statement to reflect whichever you use.


Make sure to change the defines for APIKEY, FEEDID and USERAGENT to the ones specific to your Cosm feed.

Also, feel free to change the number for MOVING_AVERAGE_NUMBER if you want to take the moving average with more (or less) than  3 samples.

At the beginning of the code, set the MAC address array to your Arduino's speccific MAc address, and set the IP that it defaults to if DHCP fails, to somehting comapatable with your local network.