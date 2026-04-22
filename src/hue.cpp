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

const unsigned char Hue::epd_bitmap_hueSwirlEye [] PROGMEM = {
  0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xc0, 0x00, 0x01, 
  0xff, 0xff, 0xf0, 0x00, 0x07, 0xff, 0xff, 0xf8, 0x00, 0x0f, 0xff, 0xff, 0xfc, 0x00, 0x1f, 0xff, 
  0xff, 0xfc, 0x00, 0x1f, 0xff, 0xff, 0xfe, 0x00, 0x3f, 0xff, 0xdf, 0xfe, 0x00, 0x3f, 0xfc, 0x03, 
  0xff, 0x00, 0x7f, 0xe0, 0x01, 0xff, 0x00, 0x7f, 0xc0, 0x00, 0xff, 0x80, 0xff, 0x80, 0xe0, 0xff, 
  0x80, 0xff, 0x83, 0xf8, 0x7f, 0xc0, 0xff, 0x0f, 0xfc, 0x7f, 0xc0, 0xff, 0x0f, 0xfc, 0x3f, 0xc0, 
  0xff, 0x1f, 0xfe, 0x3f, 0xc0, 0xff, 0x1f, 0xfe, 0x3f, 0xc0, 0xff, 0x3f, 0xfc, 0x7f, 0xc0, 0xff, 
  0x3f, 0xfc, 0x7f, 0xc0, 0xff, 0x3f, 0xf0, 0x7f, 0x80, 0xff, 0x3f, 0xf0, 0xff, 0x80, 0x7e, 0x3f, 
  0xff, 0xff, 0x80, 0x7e, 0x1f, 0xff, 0xff, 0x00, 0x18, 0x1f, 0xff, 0xff, 0x00, 0x00, 0x0f, 0xff, 
  0xfe, 0x00, 0x00, 0x0f, 0xff, 0xfc, 0x00, 0x00, 0x03, 0xff, 0xf8, 0x00, 0x00, 0x00, 0xff, 0xf0, 
  0x00, 0x00, 0x00, 0x3f, 0xc0, 0x00
};

Hue::Hue(uint8_t neoPin, uint8_t numPix)
    :strip(numPix, neoPin, NEO_GRB+NEO_KHZ800),
    r(0), g(0), b(0), numPix(numPix), epd_bitmap_allArray_LEN(0),
    faceState(BLINK), lastBlink(0), theta(0), blinkPhase(0),
    tft(TFT_CS, TFT_DC, TFT_RST),
    face(FWIDTH, FHEIGHT, &Wire, -1){

  hex[0]     = '\0'; //empty hex char array to start
  lastHex[0] = '\0';
  epd_bitmap_allArray[0] = epd_bitmap_hueSwirlEye;
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

  // config colour sensor
  // set brightness
  // 0.5x-2048x (goes up pow of 2)
  // higher gain = larger numbers recorded
  colSense.setGain(AS7343_GAIN_16X);

  //from adafruit demo code
  //dont quite understand this stuff
  //together control integration time
  //how long the sensor's photodiodes accumulate charge per measurement?
  colSense.setATIME(29);  // Integration cycles
  colSense.setASTEP(599); // Step size
  colSense.setLEDCurrent(4);
  //colSense.enableLED(true);

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
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLUE);

  //init face screen
  if(!face.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("Face screen failed to initialize!");
    return false;
  }

  face.setRotation(2);
  face.display();
  delay(2000);
  face.clearDisplay();
  
  return true;
}

void Hue::readCol(){
  colSense.enableLED(true);
  if (!colSense.readAllChannels(readings)) {
    Serial.println("Read failed!");
    return;
  }
  colSense.enableLED(false);

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
    //norm with CLEAR chan
    float clear = (float)readings[AS7343_CHANNEL_VIS_TL_0];
    float norm = (clear>0) ? 1.0/clear : 1.0;
    x *= norm;
    y *= norm;
    z *= norm;

    //standard D65 illuminant lin trans matrix
    //http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    //will give [0-1] typically
    float rLin, gLin, bLin;
    rLin =  3.2406*x - 1.5372*y - 0.4986*z;
    gLin = -0.9689*x + 1.8758*y + 0.0415*z;
    bLin =  0.0557*x - 0.2040*y + 1.0570*z;

    //clamp in case out of gamut
    rLin = constrain(rLin, 0.0, 1.0);
    gLin = constrain(gLin, 0.0, 1.0);
    bLin = constrain(bLin, 0.0, 1.0);

    //leds use lin rgb
    r = (uint8_t)(rLin*255.0);
    g = (uint8_t)(gLin*255.0);
    b = (uint8_t)(bLin*255.0);

    //apply gamma correction, lin rgb to srgb
    //https://en.wikipedia.org/wiki/SRGB#Transfer_function_(%22gamma%22)
    auto gammaCorrect = [](float lin) -> uint8_t{
      float col = (lin <= 0.0031308) ? 12.92*lin : 1.055*pow(lin, 1.0/2.4) -0.055;
      return (uint8_t)(constrain(col, 0.0, 1.0)*255.0);
    };

    uint8_t rH = gammaCorrect(rLin);
    uint8_t gH = gammaCorrect(gLin);
    uint8_t bH = gammaCorrect(bLin);

    //convert srgb to hex string, put in hex char array
    snprintf(hex, sizeof(hex), "#%02X%02X%02X", rH, gH, bH);

    return;
}

void Hue::show(){
  for(int i = 0; i < numPix; i++){
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();

  if(strcmp(hex, lastHex) != 0){
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(15, tft.height()/2);
    tft.setTextSize(5);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextWrap(true);
    tft.print(hex);
    strncpy(lastHex, hex, sizeof(lastHex));
  }

  return;
}

void Hue::express(){
  face.clearDisplay(); //clear before placing new pix
  animateFace();
  if(faceState == IDLE && millis() - lastBlink >= 5000) changeState(BLINK);
  //Serial.println(faceState);
  face.display();
}

void Hue::readMpu(){
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  prevAccel = accel;
  accel = a.acceleration;
  gyro = g.gyro;

  if((abs(gyro.x) + abs(gyro.y) + abs(gyro.z))/3.0 > 2.5){
    changeState(DIZZY);
  }
  else if(faceState == DIZZY && (abs(gyro.x) + abs(gyro.y) + abs(gyro.z))/3.0 <= 2.5){
    changeState(IDLE);
  }

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

void Hue::changeState(Faces newFace){ //default IDLE
  if(faceState == DIZZY) theta = 0; //reset theta when going FROM dizzy
  if(faceState == BLINK){
    blinkPhase = 0; //reset phase when going FROM blink
    lastBlink = millis(); //set time of last blink
  }
  if(faceState == SLEEP){
    blinkPhase = 0; //reset phase when going FROM sleep
  }
  faceState = newFace;
}

void Hue::animateFace(){
  int eyeWid = FWIDTH/5; //int mult will truncate
  int eyeHei = FHEIGHT/2;

  //base eye x pos, y is just halfway point
  int x1 = FWIDTH/2 - eyeWid*1.5;
  int x2 = FWIDTH/2 + eyeWid*1.5;

  //pos augment with accel CENTER OF EYES
  int dx1 = constrain(x1 + accel.y*4, eyeWid, FWIDTH - eyeWid*3);
  int dx2 = constrain(x2 + accel.y*4, eyeWid*3, FWIDTH - eyeWid);
  int dy = constrain(FHEIGHT/3 - accel.z*4, 20, FHEIGHT - 20);

  if(faceState == IDLE || faceState == BLINK){
    face.fillRoundRect(dx1 - eyeWid/2, dy - eyeHei/2, eyeWid, eyeHei, 90, SSD1306_WHITE);
    face.fillRoundRect(dx2 - eyeWid/2, dy - eyeHei/2, eyeWid, eyeHei, 90, SSD1306_WHITE);

    if(faceState == BLINK){
      //between eyehei/2 + 5 (most closed) and eyehei/2 + eyehei (all the way open)
      blinkPhase += 5;
      float blinkH = (sin((float)blinkPhase*0.05) + 1.0) * 0.5 * (eyeHei+10.0)+5.0;
      face.fillRoundRect(dx1-eyeWid/2, dy - eyeHei/2 + blinkH, eyeWid, eyeHei, 90, SSD1306_BLACK);
      face.fillRoundRect(dx2-eyeWid/2, dy - eyeHei/2 + blinkH, eyeWid, eyeHei, 90, SSD1306_BLACK);
      if(blinkPhase>=125){
        changeState(IDLE);
      }
    }
  }
  else if(faceState == SLEEP){
    blinkPhase += 1;
    float sleepW = sin((float)blinkPhase*0.05)*eyeWid*0.5;
    face.fillRoundRect(x1-eyeWid/2 + sleepW, eyeHei/2, eyeWid, eyeHei, 90, SSD1306_WHITE);
    face.fillRoundRect(x2-eyeWid/2 + sleepW, eyeHei/2, eyeWid, eyeHei, 90, SSD1306_WHITE);
    face.fillRoundRect(x1-eyeWid/2 + sleepW, eyeHei/2 - 5, eyeWid, eyeHei, 90, SSD1306_BLACK);
    face.fillRoundRect(x2-eyeWid/2 + sleepW, eyeHei/2 - 5, eyeWid, eyeHei, 90, SSD1306_BLACK);
  }
  else if(faceState == DIZZY){
    theta += 5;
    face.drawBitmap(x1-19, eyeHei/2, epd_bitmap_hueSwirlEye, 34, 30, SSD1306_WHITE);
    face.drawBitmap(x2-19, eyeHei/2, epd_bitmap_hueSwirlEye, 34, 30, SSD1306_WHITE);
  }
}