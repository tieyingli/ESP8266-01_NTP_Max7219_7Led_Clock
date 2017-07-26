#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <LedControl.h>

const char ssid[] = "";  //  your network SSID (name)
const char pass[] = "";       // your network password

//const char ssid[] = "";  //  your network SSID (name)
//const char pass[] = "";       // your network password

// NTP Servers:
IPAddress timeServer(17, 253, 84, 253);       //time.asia.apple.com
IPAddress timeServer1(128, 138, 141, 172); // time.nist.gov NTP server
IPAddress timeServer2(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
IPAddress timeServer3(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
IPAddress timeServer4(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov
IPAddress timeServer5(202, 120, 2, 100); // ntp.sjtu.edu.cn
const int timeZone = 8;     // China
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

String displayMessage = "";   //string on the leds
String dpBits = "";   //dots array like "11111111"
String wifiConnectingAnimations[] = { "", "1", "01", "001", "0001", "00001",
    "000001", "0000001", "00000001" };
String weekDayDots[] = { "0000000", "0000001", "1", "01", "001", "0001",
    "00001", "000001" };  //sunday is 1, monday is 2
boolean lcLedOn = false;   //if led is on. it's false before init;
LedControl lc = LedControl(2, 0, 1, 1); // For ESP-01: GPIO2-DIN, GPIO0->Clk, TXD->LOAD

time_t prevDisplay = 0; // when the digital clock was displayed
static long previousMillis = 0;

void writeToLog(String log) {
//#define _DEBUG_
#ifdef _DEBUG_
  Serial.println(log);
#endif
}

/**
 * deep sleep in seconds, can not sleep over 1 hour.
 */
void deepSleepInSeconds(float seconds) {
  writeToLog(String("... Sleep for ") + seconds + " seconds");
  delay(seconds * 1000);
  //ESP.deepSleep(seconds * 1000000);
}
/**
 * deep sleep in miliseconds, can not sleep over 1 hour.
 */
void deepSleepInMiliSeconds(float miliseconds) {
  writeToLog(String("... Sleep for ") + miliseconds + " miliseconds");
  delay(miliseconds);
  //ESP.deepSleep(seconds * 1000000);
}

/**
 * display on leds
 */
void sendToLedDisplay() {
  writeToLog("Display is " + displayMessage + " / Dots are " + dpBits);

  if (lcLedOn) {
    lc.setString(0, displayMessage, dpBits);
  }
}

/**
 * turn off leds;
 */
void turnOffLcLed() {
  if (lcLedOn) {
    writeToLog("Turning Off LED...");
    lcLedOn = false;
    lc.shutdown(0, true);  // Enable display
    writeToLog("Turning Off LED Done.");
  }
}

/**
 * turn on leds;
 */
void turnOnLcLed() {
  if (lcLedOn == false) {
    writeToLog("-- Turning On LED...");
    lcLedOn = true;
    lc.shutdown(0, false);  // Enable display
    writeToLog("-- Turning On LED Done.");
  }
}

void testLeds() {
  String chars = " _-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for (int j = 0; j < chars.length(); j++) {
    for (int i = 0; i < 8; i++) {
      lc.setChar(0, i, chars.charAt(j), (j % 2 == 0));
    }
    delay(200);
  }
}
/**
 * add a zero before the digit when digit is less then 10;
 */
String printDigits(int digits) {
  String r = String(digits);
  if (digits < 10) {
    r = "0" + r;
  }
  return r;
}

/**
 * display date time to the leds
 */
void digitalClockDisplay() {
  // digital clock display of the time
  String toDisplay = printDigits(month()) + printDigits(day())
      + printDigits(hour()) + printDigits(minute());
  displayMessage = toDisplay;
  dpBits = weekDayDots[weekday()];

  sendToLedDisplay();
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

IPAddress& getNtpServer() {
  long r = random(100000);
  int c = r % 6;
  writeToLog("Ntp Server is " + String(c) + ".");

  displayMessage = "SYNCING" + String(c);
  dpBits = "";
  sendToLedDisplay();

  switch (c) {
  case 0:
    return timeServer;
  case 1:
    return timeServer1;
  case 2:
    return timeServer2;
  case 3:
    return timeServer3;
  case 4:
    return timeServer4;
  case 5:
    return timeServer5;
  }
  return timeServer;
}

time_t getNtpTime() {
  writeToLog("-- Transmit NTP Request");

  while (Udp.parsePacket() > 0)
    ; // discard any previously received packets
  sendNTPpacket(getNtpServer());
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      writeToLog("-- Receive NTP Response");

      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long) packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long) packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long) packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long) packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  writeToLog("-- No NTP Response :-(");
  return getNtpTime(); // return 0 if unable to get the time
}
/*-------- NTP code ends ----------*/

/**
 * init LC 7 digits led
 */
void initLED() {
  writeToLog("Initing LED...");
  turnOnLcLed();
  writeToLog("-- LED Set to low brightness");
  lc.setIntensity(0, 0); // Set brightness level (0 is min, 15 is max)
  writeToLog("-- LED clear display");
  lc.clearDisplay(0);
  writeToLog("-- LED testing");
  //testLeds();
  writeToLog("-- LED clear display");
  lc.clearDisplay(0);
  writeToLog("Initing LED Done.");
  writeToLog("");
}

/**
 * init wifi
 */
void initWifi() {
  writeToLog("Initing WIFI...");
  writeToLog(String("-- Connecting to ") + String(ssid));
  WiFi.hostname("ESP8266_Clock");
  WiFi.begin(ssid, pass);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);

  int i = 1;
  while (WiFi.status() != WL_CONNECTED) {
    //animation
    displayMessage = (i%2==0) ?"CONNECT":" CONNECT";
    dpBits = wifiConnectingAnimations[i % 9];
    sendToLedDisplay();
    writeToLog(String("---- Wifi status = ") + WiFi.status());
    delay(500);

    // not connected after 1 minute
    if (i % 240 == 0) {
      // sleep for 3 mins
      displayMessage = "WIFI ERR";
      dpBits = "";
      sendToLedDisplay();
      deepSleepInSeconds(60 * 1);
    }
    i++;
  }
  writeToLog(
      String("-- IP number assigned by DHCP is ") + WiFi.localIP().toString());
  writeToLog("Initing WIFI Done.");
  writeToLog("");
}

/**
 * init ntp settings
 */
void initNtpClient() {
  writeToLog("Initing UDP...");
  Udp.begin(localPort);
  writeToLog(String("-- Local port: ") + Udp.localPort());
  writeToLog("-- Waiting for sync");
  setSyncInterval(1800); // ntp every 30 mins;
  setSyncProvider(getNtpTime);
}

void initSerial() {
#ifdef _DEBUG_
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }
  delay(1000);
  writeToLog("");
  writeToLog("Welcome to NTP Max7219 Clock");
#endif
}

void setup() {
  initSerial();
  initLED();
  initWifi();
  initNtpClient();
}

void loop() {
  previousMillis = millis();
  digitalClockDisplay();
  // turn off during night;
  if (hour() >= 22 || hour() < 6) {
    turnOffLcLed();
    //weakup every 1 hour;
    deepSleepInSeconds(60 * 60);
  } else {
    turnOnLcLed();
    //weakup every 1 mins;
    deepSleepInMiliSeconds(
        60 * 1000 - second() * 1000 - (millis() - previousMillis));
  }
}
