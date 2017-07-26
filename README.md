# ESP8266-01_NTP_Max7219_7Led_Clock

- Here is just a simple example of clock which is set up with ESP8266-01, Max7219-7-Segment-Led, NTP Protocal.
- Because I had to change the code of libraries of LedControl and Time code, see the dirs of MyLedControl and MyTimeLib for what I've done.
- The led will display infomation like 'mmddHHmm'. The dots on led panel show the weekday.
- The led will change every minute.
- The NTP client will sync with one of 5 servers every 30 minutes.
- Sorry for my pool English.

# NEXT:
- Make this little thing deep sleep for a longer battery life.
- Now it costs 80ma during syncing NTP and sending code to Max7219 and 30ma during sleeping(delaying).
- Because there is no (easy) way to force 8266 ESP-01 into deep sleep, I had to use the light sleep mode.
- I'll upload a pic of this little thing.
