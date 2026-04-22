#include "hue.h"

void IRAM_ATTR onButtonPress();
static void colourRead(void* pvParameters);
static void accelRead(void* pvParameters);
static void faceDisplay(void* pvParameters);
static void bellyDisplay(void* pvParameters);

Hue hue(25, 2);
SemaphoreHandle_t i2cMutex;
TaskHandle_t colHandle = NULL;

//ledpin = 5 -- NOT WORKING FOR INPUT
const int BUTPIN = 7;
volatile unsigned long lastButTime = 0; //isr shared var needs to be volatile
unsigned long lastPrint = 0;


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
  if(!i2cMutex){
    Serial.println("Failed to create i2cMutex!");
    while(1) delay(10);
  }
  Serial.println("i2cMutex initialized!");

  //sensor read tasks - on core 1
  xTaskCreatePinnedToCore(colourRead, "colourRead", 10000, //stack size, need to check this
                          NULL, 1, &colHandle, 1 );
  // xTaskCreatePinnedToCore(colourRead, "colourRead", 10000, //stack size, need to check this
  //                         NULL, 1, NULL, 1 );

  xTaskCreatePinnedToCore(accelRead, "accelRead", 10000,
                          NULL, 2, NULL, 1);

  //display tasks - con core 0
  xTaskCreatePinnedToCore(faceDisplay, "faceDisplay", 10000,
                          NULL, 2, NULL, 0);

  xTaskCreatePinnedToCore(bellyDisplay, "bellyDisplay", 10000,
                          NULL, 1, NULL, 0);
                          
  attachInterrupt(digitalPinToInterrupt(BUTPIN), onButtonPress, CHANGE);
}

void loop() {
  if(millis() - lastPrint >= 1000){
    hue.printRead();
    lastPrint = millis();
  }
}

void IRAM_ATTR onButtonPress(){ //isr in internal ram, not flash
  unsigned long now = millis();
  if(now-lastButTime < 100) return;
  lastButTime = now;

  BaseType_t taskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(colHandle, &taskWoken);
  portYIELD_FROM_ISR(taskWoken);
}

static void colourRead(void* pvParameters){
  for(;;){
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //sleep til button press

    while(digitalRead(BUTPIN) == LOW){
      xSemaphoreTake(i2cMutex, portMAX_DELAY);
      hue.readCol();
      xSemaphoreGive(i2cMutex);
      vTaskDelay(100/portTICK_PERIOD_MS);
    }
  }
}

static void accelRead(void* pvParameters){
  for(;;){
    xSemaphoreTake(i2cMutex, portMAX_DELAY);
    hue.readMpu(); //need to delay ~10 sec for proper readings
    xSemaphoreGive(i2cMutex);
    vTaskDelay(500/portTICK_PERIOD_MS); //outside the mutex tradeoff!
  }
}

static void faceDisplay(void* pvParameters){
  for(;;){
    xSemaphoreTake(i2cMutex, portMAX_DELAY);
    hue.express();
    xSemaphoreGive(i2cMutex);
    vTaskDelay(33/portTICK_PERIOD_MS); //~30fps
  }
}

//uses SPI, no mutex needed as only one device
static void bellyDisplay(void* pvParameters){
  for(;;){
    hue.show();
    vTaskDelay(33/portTICK_PERIOD_MS); //same here but show() only updates when hex changes
  }
}

//task sketchout for RTOS implementation
//core 1: sensor tasks
//colour sense -- read when button is pressed -- lower priority
//accelerometer sense -- check this every 500ms and then every 50 if motionn is detected -- higher priority

//core 0: display
//display face screen
//display colour val screen