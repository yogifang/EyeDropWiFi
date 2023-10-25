

#include <NTPClient.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiSettings.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <String.h>
#include <string>
#include <math.h>
#include <ESP32Time.h>
#include "ESP32_LED.h"
#include "ToF.h"
#include "handleData.h"
#include "aREST.h"
// WiFi settings
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
aREST rest = aREST();
String formattedDate;
String dayStamp;
String timeStamp;
//HTTPClient http;
WiFiServer server(80);
//// ToF sensor

#define TOF_DEBUG_ENABLE_CHARACTERISTIC_UUID 0x3001
#define TEST_CHARACTERISTIC_UUID 0x9000
#define triggerThreshold 60
#define waitToRecord 2000
float tofSmoothedAvg;
float nowValue;
int lastMillis = 0;
int dropCount = 0;
int tofBootSuccess = 0;


ESP32Time rtc;
////////////////////////////////////////////////////////////////
bool bToggle = false;

void setup() {
  EEPROM.begin(EEPROM_SIZE);
  initial_LED_Buzzer();
 // analogWrite(Buzzer, 127);
  LED(0,255,0);
  //LED(0,0,0);
  Serial.begin(115200);
  SPIFFS.begin(true); // Will format on the first run after failing to mount

  // Use stored credentials to connect to your WiFi access point.
  // If no credentials are stored or if the access point is out of reach,
  // an access point will be started with a captive portal to configure WiFi.
  WiFiSettings.connect();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  server.begin();
  Serial.println("Server started");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(28800);

tofBootSuccess = initial_tof();
  if(tofBootSuccess){
    Serial.println("TOF: initialized successfully!");
  }else{
    Serial.println("TOF: Failed to boot VL53L0X, please check the wiring.");
  }
//analogWrite(Buzzer, 0);
}

void loop() {
 // Serial.println("looping" );
  //while (!timeClient.update()) {
  ////  timeClient.forceUpdate();
  //}
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  
   // digitalWrite(Buzzer, bToggle ? HIGH : LOW);
   // bToggle = !bToggle;

  formattedDate = timeClient.getFormattedTime();
 // Serial.println(formattedDate);

 
  
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);

  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);

if(tofBootSuccess){
    
    tofSmoothedAvg = tofSensor.get(); // get avg first
    tofSensor.add(lox.readRange());  // write new value
    nowValue = tofSensor.getLast();

    if((digitalRead(getTrigger) == HIGH) && (nowValue > triggerThreshold)){
      dropCount++;
      lastMillis = millis();

      if(settings.tofDebug == '0'){
        Serial.print("Drop Count: ");
        Serial.println(dropCount);
      }

      if(settings.tofDebug == '1'){
        Serial.print(100);
        Serial.print("\t");
      }     
    }else{
      if(settings.tofDebug == '1'){
        Serial.print(0);
        Serial.print("\t");
      }
    }

   
    if((nowValue-tofSmoothedAvg) > (tofSmoothedAvg*0.2)){
    
      Serial.print(tofSmoothedAvg);   
      Serial.print("\t");
      Serial.println(nowValue);
      dropCount++ ;
    }

     if((WiFi.status() == WL_CONNECTED) && dropCount > 0){
        
            HTTPClient http;
            http.begin(WiFiSettings.restapi);
            // Data to send with HTTP POST
            char httpRequestData[256] ;
            sprintf(httpRequestData, "{\"timestamp\":%d,\"device_id\":\"%s\",\"count\":%d}", rtc.getEpoch() , WiFiSettings.device_id ,dropCount);
            dropCount = 0;
        //  sprintf(httpRequestDa ta, "{\"timestamp\":%d, \"device_id\":\"%s\"}", rtc.getEpoch(),"abc");
            Serial.println(httpRequestData);
            // Send HTTP POST request
              http.addHeader("Content-Type", "application/json");
      // Data to send with HTTP POST
            int httpResponseCode = http.POST(httpRequestData);
            if (httpResponseCode>0) {     
              String payload = http.getString();
              Serial.println(payload);
            }
            else {
          //    Serial.print("Error code: ");
          //    Serial.println(httpResponseCode);
            }
      // Free resources
          http.end();
       
     }
     
    
    }
    

  
}
