//hue.h
#pragma once
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_AS7343.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_GFX.h> 
//#include <Adafruit_ST7789.h> //belly screen lib
#include <Adafruit_SH110X.h> //replacement belly screen lib!!
#include <Adafruit_SSD1306.h> //face screen lib
class Hue{
    public:
        Hue(uint8_t neoPin = 0, uint8_t numPix = 1); //default to esp32 neopix

        bool begin(); //init everything

        //read functions
        void readCol(); //sample colour sensor and update current RGB/hex
        void readMpu(); //sample accel/gyro

        //display functions
        void show(); //push current colour to neopix and belly screen
        void express(); //update face based on accel/gyro
        void printRead(); //print current sensor readings to serial

        //current colour -- updated with readCol()
        uint8_t r, g, b; //0-255
        char hex[8]; //hex version for output to user
        char lastHex[8];

        //current accel and gyro reads -- updated with readMpu()
        sensors_vec_t accel, gyro, prevAccel, prevGyro;
        bool moving;

    private:
        //internal use vars
        uint8_t numPix;

        //sensors
        Adafruit_AS7343 colSense;
        Adafruit_MPU6050 mpu;

        //neopix and screens
        Adafruit_NeoPixel strip;
        //Adafruit_ST7789 tft; //belly screen
        Adafruit_SH1106G belly;
        Adafruit_SSD1306 face; //face screen

        //colour read util
        uint16_t readings[18]; //raw spectral reads
        void spec2rgb();

        //for processing raw colour vals
        static constexpr int NUM_VIS = 10; //like #define but type safe
        static const struct ChannelCIE{
            float xBar, yBar, zBar;
        } cieTable[NUM_VIS];

        //map the adafruit as7347 enums so we can loop
        static const uint16_t chanMap[NUM_VIS];

        //for belly screen 
        static constexpr int TFT_CS  = 14;
        static constexpr int TFT_RST = 32;
        static constexpr int TFT_DC  = 33;
        //static constexpr int TFT_LITE -- backlight pin

        //for face screen
        static constexpr int FWIDTH   = 128;
        static constexpr int FHEIGHT  = 64;
        // static constexpr int FACE_CS  = 13;
        // static constexpr int FACE_RST = 5;
        // static constexpr int FACE_DC  = 33;


        //fsm for differrennt anim
        enum Faces {IDLE, BLINK, DIZZY, SLEEP};
        Faces faceState;

        void changeState(Faces newFace = IDLE);
        void animateFace();
        void printHex();
        // void animateBelly();
        // void rotateBitmap(const unsigned char map); //TODO
        int lastBlink, theta, blinkPhase;

        // 'hueSwirlEye', 34x30px -- from image2cpp
        static const unsigned char epd_bitmap_hueSwirlEye [] PROGMEM;

        // Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 176)
        const int epd_bitmap_allArray_LEN;
        const unsigned char* epd_bitmap_allArray[1];
};