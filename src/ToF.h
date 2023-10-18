#include "Adafruit_VL53L0X.h"
#include <Smoothed.h>

/* Declare for ToF sensor */
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
Smoothed <float> tofSensor; 

// int triggerThreshold = 40;
// int dropCount = 0;
// int lastMillis = 0;


int initial_tof() {
  int counter = 0;
    /* Starting the ToF sensor */
  Serial.println("TOF: Adafruit VL53L0X test");
  while(!lox.begin()){
      counter++;
      if(counter >= 5){
        return 0;
      }else{
        Serial.println(F("TOF: Failed to boot VL53L0X"));
        Serial.println("TOF: Trying again...");
      }
      delay(1000);
  };
  lox.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED);
  lox.startRangeContinuous();   // start continuous ranging

  tofSensor.begin(SMOOTHED_AVERAGE, 32);
    
  return 1;
}
