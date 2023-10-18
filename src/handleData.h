#include <EEPROM.h>
#include <ArduinoJson.h>

#define EEPROM_SIZE             2048
#define SETTINGS_ADDRESS        0
#define DATA_ADDRESS_INDEX      8
#define DATA_ADDRESS            12


struct Settings {
  char enable;
  char tofDebug;
  unsigned long timestamp;
};

struct DataAddress {
  int index;
};

struct DataRecord {
  unsigned long timestamp;
  int data;
};

const int MAX_DATA_RECORDS = 253;  // Maximum number of data records to store
const int DATA_RECORD_SIZE = sizeof(DataRecord); // Size of each data record

Settings settings;
DataAddress dataAddress;
DataRecord dataRecords[MAX_DATA_RECORDS];
DynamicJsonDocument doc(JSON_OBJECT_SIZE(MAX_DATA_RECORDS) + MAX_DATA_RECORDS * DATA_RECORD_SIZE);

int getRecordAddress(int recordIndex) {
  return (recordIndex * sizeof(DataRecord)) + DATA_ADDRESS;
}

int getSettingsAddress(int settingsIndex) {
  return (settingsIndex * sizeof(Settings)) + SETTINGS_ADDRESS;
}

String convertDataToJSON(int block) {
  // DataRecord d[MAX_DATA_RECORDS];

  // for(int i=0; i < MAX_DATA_RECORDS; i++){
  //   EEPROM.get(DATA_ADDRESS + i * DATA_RECORD_SIZE, d[i]);
  //   Serial.printf("EEPROM[%d]: %d, %d\n", i, d[i].timestamp, d[i].data);
  // }
  
  // Create a JSON array
  JsonArray array = doc.to<JsonArray>();
  
  // Iterate through data records and add them to the array
  for (int i = DATA_ADDRESS + (block * 10); i < (DATA_ADDRESS + (block * 10) + 10); i++) {
    JsonObject dataObject = array.createNestedObject();
    dataObject["timestamp"] = dataRecords[i-DATA_ADDRESS].timestamp;
    dataObject["data"] = dataRecords[i-DATA_ADDRESS].data;
    Serial.printf("dataRecords[%d]: %d, %d\n", (i-DATA_ADDRESS), dataRecords[i-DATA_ADDRESS].timestamp, dataRecords[i-DATA_ADDRESS].data);
  }
  
  String jsonData;
  serializeJson(doc, jsonData);
  return jsonData;
}

// handle data for EEPROM
void saveDataToEEPROM() {
  for(int i=0; i < MAX_DATA_RECORDS; i++){
    EEPROM.put(DATA_ADDRESS + (i * DATA_RECORD_SIZE), dataRecords[i]);
  }
  EEPROM.commit();
  delay(10);
}

void loadDataFromEEPROM() {
  DataRecord d;
  for(int i=0; i < MAX_DATA_RECORDS; i++){
    EEPROM.get(DATA_ADDRESS + i * DATA_RECORD_SIZE, d);
    dataRecords[i] = d;
    // Serial.printf("EEPROM Load[%d]: %d, %d\n", i, d.timestamp, d.data);
  }
}

void loadDataIndexFromEEPROM() {
  DataAddress d;
  EEPROM.get(DATA_ADDRESS_INDEX, d);
  dataAddress.index = d.index;
  Serial.printf("EEPROM Load [Data Index]: %d\n", dataAddress.index);
}

void saveSingleDataToEEPROM(unsigned long timestamp, int data) {
  loadDataIndexFromEEPROM();

  // Check if we have reached the maximum number of data records
  if (dataAddress.index >= MAX_DATA_RECORDS) {
    // If so, reset the index to 0
    dataAddress.index = 0;
  }

  // Save data to EEPROM
  dataRecords[dataAddress.index].timestamp = timestamp;
  dataRecords[dataAddress.index].data = data;
  Serial.println(DATA_ADDRESS + (dataAddress.index * DATA_RECORD_SIZE));
  EEPROM.put(DATA_ADDRESS + (dataAddress.index * DATA_RECORD_SIZE), dataRecords[dataAddress.index]);
  EEPROM.commit();
  delay(10);

  // Save index to EEPROM
  dataAddress.index++;
  EEPROM.put(DATA_ADDRESS_INDEX, dataAddress);
  EEPROM.commit();
  delay(10);
}

void generateDummyData(){
  for (int i=0; i < MAX_DATA_RECORDS; i++) {
    dataRecords[i].timestamp = 1609459200 + i; // 使用 millis() 函式獲取時間戳記
    // dataRecords[i].data = random(256); // 使用 random() 函式生成 0 到 255 的隨機數
    dataRecords[i].data = i; // 使用 random() 函式生成 0 到 255 的隨機數
  }

  for(int i=0; i < MAX_DATA_RECORDS; i++){
    EEPROM.put(DATA_ADDRESS + (i * DATA_RECORD_SIZE), dataRecords[i]);
    Serial.printf("EEPROM Gen[%d]: %d, %d\n", i, dataRecords[i].timestamp, dataRecords[i].data);
  }
  
  EEPROM.commit();
  delay(10);
}


// handle settings for EEPROM
void saveSettingsToEEPROM() {
  EEPROM.put(SETTINGS_ADDRESS, settings);
  // EEPROM.put(SETTINGS_ADDRESS+1, settings.tofDebug);
  delay(10);
  EEPROM.commit();
}

void loadSettingsFromEEPROM() {
  Settings s;
  EEPROM.get(SETTINGS_ADDRESS, s);
  // EEPROM.get(SETTINGS_ADDRESS+1, s.tofDebug);
  settings.enable = s.enable;
  settings.tofDebug = s.tofDebug;
  settings.timestamp = s.timestamp;
  Serial.printf("EEPROM Load [Enable] Settings: %c\n", s.enable);
  Serial.printf("EEPROM Load [ToF Debug] Settings: %c\n", s.tofDebug);
  Serial.printf("EEPROM Load [ToF Debug] Settings: %c\n", s.timestamp);
  delay(10);
}