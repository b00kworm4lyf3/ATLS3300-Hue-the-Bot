//hue.cpp
#include "hue.h"

//CIE 1931 2° color matching functions (X̄, Ȳ, Z̄)
//at AS7343 channel center wavelengths
//sourced from https://cie.co.at/datatable/cie-1931-colour-matching-functions-2-degree-observer
const Hue::ChannelCIE Hue::cieTable[NUM_VIS] = {
  {0.02319,	0.00064, 0.1102},     // 405nm F1 
  {0.21477,	0.0073,  1.03905},    // 425nm F2
  {0.3362,	0.038,   1.77211},    // 450nm FZ
  {0.1421,	0.1126,  1.0419},     // 475nm F3
  {0.0291,	0.6082,  0.1117},     // 515nm F4
  {0.43345,	0.99495, 0.00875},    // 550nm F5
  {0.51205, 1.0000,  0.00575},    // 555nm FY
  {1.0622,  0.6310,  0.0008},     // 600nm FXL
  {0.4479,	0.175,	 0.00002},    // 640nm F6
  {0.0227,	0.00821, 0.0000},     // 690nm F7
};

//map of reading enums to loop through in order
//enums not necessarily in order so solidifying that here
const uint16_t Hue::chanMap[NUM_VIS] = {
  AS7343_CHANNEL_F1, AS7343_CHANNEL_F2,
  AS7343_CHANNEL_FZ, AS7343_CHANNEL_F3,
  AS7343_CHANNEL_F4, AS7343_CHANNEL_F5,
  AS7343_CHANNEL_FY, AS7343_CHANNEL_FXL,
  AS7343_CHANNEL_F6, AS7343_CHANNEL_F7,
};

//
Hue::Hue(uint8_t neoPin, uint8_t numPix)
    :strip(numPix, neoPin, NEO_GRB+NEO_KHZ800),
    r(0), g(0), b(0), numPix(numPix),
    tft(TFT_CS, TFT_DC, TFT_RST),
    face(FWIDTH, FHEIGHT, &Wire, -1){

  hex[0] = '\0'; //empty hex char array to start
  return;
}

bool Hue::begin(){
  strip.begin();
  strip.setBrightness(30);
  strip.show();

  if(!colSense.begin()){
    Serial.println("Colour sensor failed to initialize!");
    return false;
  }

  //config colour sensor
  //set brightness
  //0.5x-2048x (goes up pow of 2)
  //higher gain = larger numbers recorded
  colSense.setGain(AS7343_GAIN_16X);

  //from adafruit demo code
  //dont quite understand this stuff
  //together control integration time
  //how long the sensor's photodiodes accumulate charge per measurement?
  colSense.setATIME(29);  // Integration cycles
  colSense.setASTEP(599); // Step size
  colSense.setLEDCurrent(4);
  colSense.enableLED(true);

  if(!mpu.begin()){
    Serial.println("Gyro-accel sensor failed to initialize!");
    return false;
  }

  //config mpu sensor
  //set max g-force measure
  //2G-16G, lower more precise/higher more impact
  //4G = ~+- 9.8m/s^2 * 4 = ~+-39.2m/s^2
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G); 

  //set max rot speed
  //250deg-2000deg, lower more precise
  //500deg = +-500deg/s but gyro vals are rad/sec! so read +-8.7rad/sec
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  //set filter bandwidth
  //smooth noise and vib, filters out anything changing faster
  //5hz-260hz, lower is smoother but laggier/higher is more responsive but noisier
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  //init ST7789 240x240 (belly screen)
  tft.init(240, 240);
  tft.fillScreen(ST77XX_BLACK);

  //init face screen
  if(!face.begin(SSD1306_SWITCHCAPVCC, FADDR)){
    Serial.println("Face screen failed to initialize!");
    return false;
  }

  face.display();
  delay(1000);
  face.clearDisplay();
  
  return true;
}

void Hue::readCol(){
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
  for(int i = 0; i < numPix; i++){
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();

  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.print(hex);

  return;
}

void Hue::express(){
  face.clearDisplay(); //clear before placing new pix

  //pix pos
  int px = constrain(FWIDTH/2  + accel.x, 0, FWIDTH  - 1);
  int py = constrain(FHEIGHT/2 + accel.y, 0, FHEIGHT - 1);

  face.drawPixel(px, py, SSD1306_WHITE);
  face.display();
}

void Hue::readMpu(){
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  accel = a.acceleration;
  gyro = g.gyro;

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

  //print accel and gyro reads
  Serial.println("Accel (x(L/R), y(F/B), z(U/D)): ");
  Serial.println(String(accel.x) + ", " + String(accel.y) + ", " + String(accel.z));
  Serial.println("Gyro (x(pitch), y(roll), z(yaw)): ");
  Serial.println(String(gyro.x) + ", " + String(gyro.y) + ", " + String(gyro.z));

}