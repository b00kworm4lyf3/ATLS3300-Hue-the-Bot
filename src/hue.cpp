//hue.cpp
#include "hue.h"

//CIE 1931 2° color matching functions (X̄, Ȳ, Z̄)
//at AS7343 channel center wavelengths
const Hue::ChannelCIE Hue::cieTable[NUM_VIS] = {
  {0.0232, 0.0006, 0.1102},  // 405nm F1
  {0.1096, 0.0040, 0.5359},  // 425nm F2
  {0.3362, 0.0380, 1.7721},  // 450nm FZ
  {0.1421, 0.0586, 1.0507},  // 475nm F3
  {0.0093, 0.1693, 0.2080},  // 515nm F4
  {0.4334, 0.9950, 0.0087},  // 550nm F5
  {0.4929, 1.0000, 0.0039},  // 555nm FY
  {1.0622, 0.6310, 0.0008},  // 600nm FXL
  {0.1750, 0.1070, 0.0000},  // 640nm F6
  {0.0057, 0.0021, 0.0000},  // 690nm F7
};

//map of reading enums to loop through in order
const uint16_t Hue::chanMap[NUM_VIS] = {
  AS7343_CHANNEL_F1, AS7343_CHANNEL_F2,
  AS7343_CHANNEL_FZ, AS7343_CHANNEL_F3,
  AS7343_CHANNEL_F4, AS7343_CHANNEL_F5,
  AS7343_CHANNEL_FY, AS7343_CHANNEL_FXL,
  AS7343_CHANNEL_F6, AS7343_CHANNEL_F7,
};

Hue::Hue(uint8_t neoPin, uint8_t numPix)
    :strip(numPix, neoPin, NEO_GRB+NEO_KHZ800),
    r(0), g(0), b(0){
      
  hex[0] = '\0'; //empty hex char array to start
  return;
}

bool Hue::begin(){ //originally what was in main setup()
  strip.begin();
  strip.setBrightness(30);
  strip.show();

  if(!colSense.begin()) return false;

  // Configure sensor
  colSense.setGain(AS7343_GAIN_64X);
  colSense.setATIME(29);  // Integration cycles
  colSense.setASTEP(599); // Step size

  return true;
}

void Hue::read(){
  if (!colSense.readAllChannels(readings)) {
    Serial.println("Read failed!");
    return;
  }

  spec2rgb();

  return;
}

void Hue::spec2rgb(){
    //sum vals into xyz
    float x = 0, y = 0, z = 0;
    for(int i = 0; i < NUM_VIS; i++){
      float val = (float)readings[chanMap[i]]; //read in mapped order
      x += val*cieTable[i].xBar;
      y += val*cieTable[i].yBar;
      z += val*cieTable[i].zBar;
    }

    //xyz to lin rgb
    //norm y to = 1 (y is luminance)
    float norm = (y>0) ? 1.0/y : 1.0;
    x *= norm;
    y *= norm; //will be 1 by def
    z *= norm;

    //standard D65 illuminant lin trans matrix 
    //will give [0-1] typically
    float rLin, gLin, bLin;
    rLin =  3.2406*x - 1.5372*y - 0.4986*z;
    gLin = -0.9689*x + 1.8758*y + 0.0415*z;
    bLin =  0.0557*x - 0.2040*y + 1.0570*z;

    //clamp in case out of gamut
    rLin = constrain(rLin, 0.0, 1.0);
    gLin = constrain(gLin, 0.0, 1.0);
    bLin = constrain(bLin, 0.0, 1.0);

    r = (uint8_t)(rLin*255.0);
    g = (uint8_t)(gLin*255.0);
    b = (uint8_t)(bLin*255.0);

    //convert rgb to hex string, put in hex char array
    snprintf(hex, sizeof(hex), "#%02X%02X%02X", r, g, b);

    return;
}

void Hue::show(){
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();

  return;
}

void Hue::printRead(){
  Serial.println("\n--- Spectral Readings ---");

  const char* labels[] = {
    "F1 (405nm)", "F2 (425nm)", "FZ (450nm)",
    "F3 (475nm)", "F4 (515nm)", "F5 (550nm)",
    "FY (555nm)", "FXL (600nm)", "F6 (640nm)", "F7 (690nm)"
  };

  //print spectral readings
  for(int i = 0; i < NUM_VIS; i++){
    Serial.print(labels[i]);
    Serial.print(": ");
    Serial.println(readings[chanMap[i]]);
  }

  //print rgb and hex vals
  Serial.println("Colour (hex): " + String(hex));
  Serial.println("RGB: " + String(r) + ", " + String(g) + ", " + String(b));
}