Project by Andrew Merczynski-Hait, andrewmh20@nyc.rr.com, 
Based off of the project by Lady Ada, http://www.ladyada.net/make/tweetawatt/index.html


This code is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported. This code and modifications must be attributed to "Andrew Merczynski-Hait". Please see the attached license file.

With the understanding that the license limits certain activities on the part of the licensee, and that without express permission from Andrew Merczynski-Hait the terms of the license may not be violated:

1)If one wishes to modify the software without sharing it open-source, then I must grant explicit permission to do so, and **I may be likely to do so.**
2)The full license text must be included in any distributions or modifications of this code.




**Harware Instructions:**

I had an extra Xbee adapter board on hand, so for one used as the receiver I soldered the headers on it as Lady Ada suggests to use the FTDI cable for communication directly with the computer. However, for the second one, and the one I used in the end for the receiver I soldered vertical headers, and only on the pins I needed in order to facilitate better connection with the Arduino.

**Instructions for installation:**

I diverged from the instructions for Lady Ada's Tweet-a-Watt when I used analog input 0 and 1 on the transmitter Xbee, instead of analog input 0 and 4. I think the code should work for either one, but if there is a problem, please contact me.

Make sure to change the defines for APIKEY, FEEDID and USERAGENT to the ones specific to your Cosm feed.

Also, feel free to change the number for MOVING_AVERAGE_NUMBER if you want to take the moving average with more (or less) than  3 samples.

At the beginning of the code, set the MAC address array to your Arduino's speccific MAc address, and set the IP that it defaults to if DHCP fails, to somehting comapatable with your local network.

Additional Features:
Included in comments is code for calculating the power factor. I was not able to get it accurate enough to be useful, and I provide no guarentee that the code currently in place, albeit commented out, will work.

As of yet there is no support for multiple transmitters.