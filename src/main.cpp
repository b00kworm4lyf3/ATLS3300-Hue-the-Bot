#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_AS7343.h>
const int NEOPIN = 0;

Adafruit_NeoPixel strip(1, NEOPIN, NEO_GRB + NEO_KHZ800);
Adafruit_AS7343 colSense;

void setup() {
  Serial.begin(9600);

  strip.begin();
  strip.setBrightness(30);
  strip.show();

  if (!colSense.begin()) {
    Serial.println("Could not find AS7343 sensor!");
    while (1) {
      delay(10);
    }
  }

  Serial.println("AS7343 found!");

  // Configure sensor
  colSense.setGain(AS7343_GAIN_64X);
  colSense.setATIME(29);  // Integration cycles
  colSense.setASTEP(599); // Step size

  Serial.print("Integration time: ");
  Serial.print(colSense.getIntegrationTime());
  Serial.println(" ms");
}

void loop() {
  uint16_t readings[18];

  // Read all channels (starts measurement, waits, reads internally)
  if (!colSense.readAllChannels(readings)) {
    Serial.println("Read failed!");
    delay(500);
    return;
  }

  // Print spectral channels (wavelength order)
  Serial.println("\n--- Spectral Readings ---");

  Serial.print("F1  (405nm violet):     ");
  Serial.println(readings[AS7343_CHANNEL_F1]);

  Serial.print("F2  (425nm violet-blue):");
  Serial.println(readings[AS7343_CHANNEL_F2]);

  Serial.print("FZ  (450nm blue):       ");
  Serial.println(readings[AS7343_CHANNEL_FZ]);

  Serial.print("F3  (475nm blue-cyan):  ");
  Serial.println(readings[AS7343_CHANNEL_F3]);

  Serial.print("F4  (515nm green):      ");
  Serial.println(readings[AS7343_CHANNEL_F4]);

  Serial.print("F5  (550nm green-yel):  ");
  Serial.println(readings[AS7343_CHANNEL_F5]);

  Serial.print("FY  (555nm yellow-grn): ");
  Serial.println(readings[AS7343_CHANNEL_FY]);

  Serial.print("FXL (600nm orange):     ");
  Serial.println(readings[AS7343_CHANNEL_FXL]);

  Serial.print("F6  (640nm red):        ");
  Serial.println(readings[AS7343_CHANNEL_F6]);

  Serial.print("F7  (690nm deep red):   ");
  Serial.println(readings[AS7343_CHANNEL_F7]);

  Serial.print("F8  (745nm near-IR):    ");
  Serial.println(readings[AS7343_CHANNEL_F8]);

  Serial.print("NIR (855nm near-IR):    ");
  Serial.println(readings[AS7343_CHANNEL_NIR]);

  // Print clear/VIS channels (one from each cycle)
  Serial.print("VIS (clear):            ");
  Serial.println(readings[AS7343_CHANNEL_VIS_TL_0]);

  delay(500);

  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
}