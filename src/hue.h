//hue.h
#pragma once
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_AS7343.h>
#include <Adafruit_MPU6050.h>

class Hue{
    public:
        Hue(uint8_t neoPin = 0, uint8_t numPix = 1); //only builtin neopix rn

        bool begin();
        void readCol(); //sample colour sensor and update current RGB/hex
        void readMpu(); //sample accel/gyro
        void show(); //push current colour (rn to esp32 neopix, will be to screen and 2 external neopix)
        void printRead(); //print to serial

        //current colour -- updated with readCol()
        uint8_t r, g, b; //0-255
        char hex[8]; //hex version

        //current accel and gyro reads -- updated with readMpu()
        sensors_vec_t accel, gyro;

    private:
        Adafruit_AS7343 colSense;
        Adafruit_MPU6050 mpu;
        Adafruit_NeoPixel strip;
        uint16_t readings[18]; //raw spectral reads
        
        void spec2rgb();

        //for processing raw colour vals
        static constexpr int NUM_VIS = 10; //like #define but type safe
        static const struct ChannelCIE{
            float xBar, yBar, zBar;
        } cieTable[NUM_VIS];

        static const uint16_t chanMap[NUM_VIS]; //map the adafruit as7347 enums so we can loop
};