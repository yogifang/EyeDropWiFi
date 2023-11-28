

#include <NTPClient.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiSettings.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <HTTPClient.h>



#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <SPIFFS.h>
#include <String.h>
#include <math.h>
#include <ESP32Time.h>
#include "ESP32_LED.h"
#include "ToF.h"
#include "handleData.h"
#include "aREST.h"
#include <Ticker.h>
#include "UUID.h"
#include "uuids300.h"

void cloudHttp();

// WiFi settings
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
aREST rest = aREST();
String formattedDate;
String dayStamp;
String timeStamp;
//HTTPClient http;
WiFiServer server(80);
AsyncWebServer aserver(80);

AsyncWebSocket ws("/ws");

#include "pages.html"
//// ToF sensor

#define TOF_DEBUG_ENABLE_CHARACTERISTIC_UUID 0x3001
#define TEST_CHARACTERISTIC_UUID 0x9000
#define triggerThreshold 60
#define waitToRecord 2000
#define Buzzer A2
#define getTrigger RX
#define pushEnable TX

uint16_t tofSmoothedAvg;
uint16_t nowValue;
uint16_t realTimeValue;
int lastMillis = 0;
int dropCount = 0;
int tofBootSuccess = 0;

bool ledState = 0;
const int ledPin = 2;

UUID uuid;
ESP32Time rtc;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

////////////////////////////////////////////////////////////////
bool bToggle = false;
Ticker ticker;
long lastTime = 0;

hw_timer_t *My_timer = NULL;
portMUX_TYPE mux0 = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer(){
  noInterrupts();
 portENTER_CRITICAL(&mux0); //要鎖住記憶體區塊,不然跑一跑就爆掉了
// timerAlarmDisable(My_timer);
 //   realTimeValue = tofBootSuccess ?  lox.readRange() : 0.0;
 //timerAlarmEnable(My_timer);
  portEXIT_CRITICAL(&mux0); 
  interrupts();
  
}

void notifyClients() {
  ws.textAll(String(ledState));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "update") == 0) {
      ledState = !ledState;
      notifyClients();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  Serial.println("WebSocket event started.");
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  aserver.addHandler(&ws);
  
}

String processor(const String& var){
  Serial.println(var);
   return WiFiSettings.suuid;
 
}



////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {
  EEPROM.begin(EEPROM_SIZE);
  initial_LED_Buzzer();
  analogWrite(Buzzer, 127);
  // LED(0,255,0);
  LED(0,0,0);
  Serial.begin(115200);

  SPIFFS.begin(true); // Will format on the first run after failing to mount

  pinMode(getTrigger, INPUT);
  pinMode(pushEnable, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  digitalWrite(pushEnable, HIGH);

 // while(1){
  //   analogWrite(Buzzer, 127);
  //   delay(200);
  //   analogWrite(Buzzer, 0);
  //   delay(300);
  //   Serial.print(".");
  //}


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
  

   Serial.println(WiFiSettings.suuid);
   Serial.println(WiFi.localIP());
   Serial.println(WiFi.getHostname());
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(0);
ticker.attach(5, cloudHttp);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  tofBootSuccess = initial_tof();
  if(tofBootSuccess){
    Serial.println("TOF: initialized successfully!");
  }else{
    Serial.println("TOF: Failed to boot VL53L0X, please check the wiring.");
  }
  server.close();
  aserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
   request->send_P(200, "text/html", index_html, processor);
  });

  
  aserver.on("/uuid", HTTP_GET, [](AsyncWebServerRequest *request) {
    String sIdx;
     if (request->hasParam("uuid"))
    {
        sIdx = request->getParam("uuid")->value();
    }
   // Serial.println("uuid----------------");
    String strUUID = uuidxx[sIdx.toInt()] ;
    WiFiSettings.setuuid(strUUID);
    request->send(200, "text/plain",strUUID );
  });

  AsyncElegantOTA.begin(&aserver);    // Start ElegantOTA

aserver.begin() ;

  digitalWrite(pushEnable, HIGH);

}



void loop() {
  
  ws.cleanupClients();
     

if(lox.isRangeComplete() || tofBootSuccess){ 
    tofSmoothedAvg = tofSensor.get(); // get avg first
    nowValue = lox.readRange(); // get new value  
    uint16_t distance = lox.readRange();
    
    if(((nowValue-tofSmoothedAvg) > 7) && tofSmoothedAvg > 0){
      Serial.print(tofSmoothedAvg);   
      Serial.print("\t");
      Serial.println(distance);
      dropCount++ ;
      analogWrite(Buzzer, 128);
     delay(200);
     analogWrite(Buzzer, 0);
     delay(300);

    } else {
       tofSensor.add(nowValue);  // write new value
      // Serial.print("+" );
    }
  }
}

void cloudHttp() {
  
   formattedDate = timeClient.getFormattedTime();
 // Serial.println(formattedDate);
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);

  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
//  Serial.println(WiFiSettings.suuid);
  if((WiFi.status() == WL_CONNECTED) && dropCount > 0){
           HTTPClient http;
            http.begin(WiFiSettings.restapi);
            // Data to send with HTTP POST
            char httpRequestData[256] ;
            
            sprintf(httpRequestData, "{\"timestamp\":%d,\"device_id\":\"%s\",\"count\":%d}", rtc.getEpoch() , WiFiSettings.suuid.c_str() ,dropCount);
//        Serial.println(httpRequestData);
            dropCount = 0;
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