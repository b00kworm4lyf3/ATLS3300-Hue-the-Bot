#include "hue.cpp";

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
  hue.read();
  hue.printRead();
  hue.show();
  delay(500);
}