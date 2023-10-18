
#define LED_r 10
#define LED_w 9
#define LED_g 8
#define Buzzer A2
#define getTrigger RX
#define pushEnable TX
// #define Buzzer A2

void LED(int r, int w, int g){
  analogWrite(LED_r, r);
  analogWrite(LED_w, w);
  analogWrite(LED_g, g);
}

void initial_LED_Buzzer() {
  // LEDs
  pinMode(LED_r, OUTPUT);
  pinMode(LED_w, OUTPUT);
  // pinMode(LED_w, INPUT);
  pinMode(LED_g, OUTPUT);
  pinMode(getTrigger, INPUT);
  
  pinMode(pushEnable, OUTPUT);
  digitalWrite(pushEnable, HIGH);

  pinMode(Buzzer, OUTPUT);
  
  LED(128, 0, 0);
}

void device_on(){
  analogWrite(Buzzer, 4);
  delay(100);
  analogWrite(Buzzer, 0);
  delay(10);
  analogWrite(Buzzer, 4);
  delay(100);
  analogWrite(Buzzer, 0);
  delay(10);
}

void device_off(){
  analogWrite(Buzzer, 4);
  delay(1000);
  analogWrite(Buzzer, 0);
}