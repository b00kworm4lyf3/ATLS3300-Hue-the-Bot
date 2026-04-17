#include "hue.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static void colourRead(void* pvParameters);
static void accelRead(void* pvParameters);
static void faceDisplay(void* pvParameters);
static void bellyDisplay(void* pvParameters);

Hue hue(25, 2);
SemaphoreHandle_t i2cMutex;
const int BUTPIN = 26;


void setup() {
  Serial.begin(9600);

  pinMode(BUTPIN, INPUT_PULLUP);

  if(!hue.begin()){
    Serial.println("Hue failed to initialize!");
    while(1) delay(10);
  }
  Serial.println("Hue is initialized!");

  //mutual exclusion for i2c (3 devices on i2c)
  i2cMutex = xSemaphoreCreateMutex();

  //sensor read tasks - on core 1
  xTaskCreatePinnedToCore(colourRead, "colourRead", 10000, //stack size, need to check this
                          NULL, 1, NULL, 1 );

  xTaskCreatePinnedToCore(accelRead, "accelRead", 10000,
                          NULL, 2, NULL, 1);

  //display tasks - con core 0
  xTaskCreatePinnedToCore(faceDisplay, "faceDisplay", 10000,
                          NULL, 2, NULL, 0);

  xTaskCreatePinnedToCore(bellyDisplay, "bellyDisplay", 10000,
                          NULL, 1, NULL, 0);
  
}

void loop() {
  int time = millis();
  if(time%50 <= 10) hue.printRead();
  //delay(500);
}

static void colourRead(void* pvParameters){
  for(;;){
    xSemaphoreTake(i2cMutex, portMAX_DELAY);
    hue.readCol(); //need to connect to button at some point
    xSemaphoreGive(i2cMutex);
    vTaskDelay(500/portTICK_PERIOD_MS); //outside the mutex tradeoff!
  }
}

static void accelRead(void* pvParameters){
  for(;;){
    xSemaphoreTake(i2cMutex, portMAX_DELAY);
    hue.readMpu(); //need to delay ~10 sec for proper readings
    xSemaphoreGive(i2cMutex);
    vTaskDelay(500/portTICK_PERIOD_MS);
  }
}

static void faceDisplay(void* pvParameters){
  for(;;){
    xSemaphoreTake(i2cMutex, portMAX_DELAY);
    hue.express();
    xSemaphoreGive(i2cMutex);
    vTaskDelay(500/portTICK_PERIOD_MS);
  }
}

static void bellyDisplay(void* pvParameters){
  for(;;){
    hue.show();
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