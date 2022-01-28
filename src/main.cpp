#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//#include <sntp.h>
//#include <time.h>

// The ESP-12E RTC resets upon entering deep sleep. The RTC is also very innacurate
// Light sleep has a max length of ~4.5 mins
// The RTC cannot be set, only read. A possible solution would be to store an RTC offset along with the last read UTC time and do fancy maffs.


// TODO: Format strings to follow 00:00:00 format

const char* ssid = "";
const char* pass = "";

WiFiUDP client;
NTPClient timeClient(client, "pool.ntp.org");

const int timeOffset = -18000; // EST

const int sleepInterval = 5; // sleep length in seconds
const int checkInterval = 30; // how many seconds until re-aquire RTC

typedef struct {
  long rtcOffset;
  long lastFetch;
} rtcVars;
rtcVars readFromRTC;

struct timeval tv;

void connectWiFi() {
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\nConnected: ");
  Serial.println(WiFi.localIP());
}

// return the approximate current time
long calcEpoch() {
  return readFromRTC.lastFetch + ((readFromRTC.rtcOffset + system_get_rtc_time()) / 1000 / 1000);
}

int calcHour() {
  return int(calcEpoch() % 86400 / 60 / 60);
}

int calcMinute() {
  return int(calcEpoch() % 3600 / 60);
}

int calcSecond() {
  return calcEpoch() % 60;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  system_rtc_mem_read(65, &readFromRTC, sizeof(readFromRTC));

  //Serial.println(readFromRTC.lastFetch);
  //Serial.println(readFromRTC.rtcOffset);

  if (calcEpoch() >= readFromRTC.lastFetch + checkInterval || (readFromRTC.lastFetch <= 0 || readFromRTC.rtcOffset <= 0)) {
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }

    // get new rtc
    timeClient.begin();
    timeClient.update();
    Serial.println("Updated RTC");
    
    readFromRTC.lastFetch = 0;
    readFromRTC.rtcOffset = 0;

    readFromRTC.lastFetch = timeClient.getEpochTime() + timeOffset;
  }

  Serial.println("Current Time: " + String(calcHour()) + ":" + String(calcMinute()) + ":" + String(calcSecond()));

  readFromRTC.rtcOffset += system_get_rtc_time() + (sleepInterval*1000*1000);
  system_rtc_mem_write(65, &readFromRTC, sizeof(readFromRTC));
  ESP.deepSleep(sleepInterval*1000*1000);
}

void loop() {}