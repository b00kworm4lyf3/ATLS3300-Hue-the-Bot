#include "hue.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void colourRead(void* pvParameters);

//using default constructor with built-in neopixel
//update with (pin#, 2) once extern neopixels are added
Hue hue(25, 2);



void setup() {
  Serial.begin(9600);

  if(!hue.begin()){
    Serial.println("Hue failed to initialize!");
    while(1) delay(10);
  }
  Serial.println("Hue is initialized!");

  xTaskCreatePinnedToCore(
            colourRead,
            "colourRead",
            10000, //stack size, need to check this
            NULL,
            1,
            NULL,
            1
          );
}

void loop() {
  hue.readMpu(); //need to delay ~10 sec for proper readings
  int time = millis();
  if(time%50 == 0) hue.printRead();
  hue.show();
  hue.express();
  //delay(500);
}

void colourRead(void* pvParameters){
  for(;;){
    //hue.readCol(); //need to connect to button at some point
    vTaskDelay(500/portTICK_PERIOD_MS);
  }
}

//task sketchout for RTOS implementation
//core 1: sensor tasks
//colour sense -- read when button is pressed -- lower priority
//accelerometer sense -- check this every 500ms and then every 50 if motionn is detected -- higher priority

//core 0: display
//display face screen
//display colour val screen