#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
int led = LED_BUILTIN;
const int NEOPIN = 0;

Adafruit_NeoPixel strip(1, NEOPIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);

  pinMode(led, OUTPUT);

  strip.begin();
  strip.setBrightness(30);
  strip.show();
}

void loop() {
  // Say hi!
  Serial.println("Hello from VS Code!");
  
  //digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
  delay(500);                // wait for a half second

  //digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
  delay(500);                // wait for a half second
}