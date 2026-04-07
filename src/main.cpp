#include "hue.h"

//using default constructor with built-in neopixel
//update with (pin#, 2) once extern neopixels are added
Hue hue(12, 2); 

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
  hue.express();
  delay(500);
}

//task sketchout for RTOS implementation
//core 1: sensor tasks
//colour sense -- read when button is pressed -- lower priority
//accelerometer sense -- check this every 500ms and then every 50 if motionn is detected -- higher priority

//core 0: display
//display face screen
//display colour val screen