Project by Andrew Merczynski-Hait, andrewmh20@nyc.rr.com;
Based off of the project by Lady Ada, http://www.ladyada.net/make/tweetawatt/index.html
See https://www.andrewmh20.github.com/Arduino-Wireless-Power-Meter for documentation

If you wish to obtain a copy of my notebook taken during this process please email me about it.

This code is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported. This code and modifications must be attributed to "Andrew Merczynski-Hait". Please see the attached license file.

With the understanding that the license limits certain activities on the part of the licensee and that without express permission from Andrew Merczynski-Hait the terms of the license may not be violated:

1) If one wishes to modify the software without sharing it open-source, then I must grant explicit permission to do so, and **I may be very likely to do so, especially if I am aware of the modifications.**
2) The full license text must be included in any distributions or modifications of this code.



**Hardware Instructions:**

Instructions can be found for the Tweet-a-Watt at http://www.ladyada.net/make/tweetawatt/index.html. 

I had an extra Xbee adapter board on hand, so for one used as the receiver I soldered the headers on it as Lady Ada suggests to use the FTDI cable for communication directly with the computer. However, for the second one, and the one I used in the end for the receiver I soldered vertical headers, and only on the pins I needed in order to facilitate better connection with the Arduino. 

I diverged from the instructions for Lady Ada's Tweet-a-Watt when I used analog input 0 and 1 on the transmitter Xbee, instead of analog input 0 and 4. I think the code I wrote should work for either one, but if there is a problem, please contact me.

**Instructions for installation:**

Make sure to change the defines for APIKEY, FEEDID and USERAGENT to the ones specific to your Cosm feed.

Also, feel free to change the number for MOVING_AVERAGE_NUMBER if you want to take the moving average with more (or less) than 3 samples.

At the beginning of the code, set the MAC address array to your Arduino's specific MAC address, and set the IP that it defaults to if DHCP fails, to something compatible with your local network.

**Notes:**

As of yet there is no support for multiple transmitters.
Also, the checksum is not calculated.

While building this I had specific difficulty getting the Xbees setup. I have found that it is often necessary to keep the Xbee out of its socket before uploading, and then when prompted place it in its socket. Also, you may need to just switch computers, and keep keep trying even if you don't know why it’s not working...

Also, because of all the strings I was printing I found that I was running out of RAM on the ATMEGA328 so I put many of them in Flash memory. That may be the culprit if the whole thing seems to crash with the addition of one extra line.

**Additional Features:**

Included in comments is code for calculating the power factor. I was not able to get it accurate enough to be useful and I provide no guarantee that the code currently in place, albeit commented out, will work.
