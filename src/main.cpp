#include "hue.h"

Hue hue;

void setup() {
  Serial.begin(9600);

  if(!hue.begin()){
    Serial.println("Hue failed to initialize!");
    while(1) delay(10);
  }
  Serial.println("Hue is initialized!");
}

void loop() {
  hue.readCol();
  hue.readMpu(); //need to delay ~10 sec for proper readings
  hue.printRead();
  hue.show();
  delay(500);
}

//task sketchout
//core 1: sensor tasks
//colour sense -- read when button is pressed -- lower priority
//accelerometer sense -- check this every 500ms and then every 50 if motionn is detected -- higher priority

//core 0: display
//display face screen
//display colour val screen