//Spectral measurements to RGB (for use as LED vals and user output)
//and hex codes (for user output)
#include <Arduino.h>

struct ChannelCIE {
  float xBar;
  float yBar;
  float zBar;
};

//CIE 1931 2° color matching functions (X̄, Ȳ, Z̄)
//at AS7343 channel center wavelengths
const ChannelCIE cieTable[10] = {
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

void convertSpec(float *visChans, uint8_t *rgb){
    //sum vals into xyz
    float x = 0, y = 0, z = 0;
    for(int i = 0; i < 10; i++){
        x += visChans[i]*cieTable[i].xBar;
        y += visChans[i]*cieTable[i].yBar;
        z += visChans[i]*cieTable[i].zBar;
    }

    //xyz to lin rgb
    //norm y to = 1 (y is luminance)
    float norm = (y>0) ? 1.0/y : 1.0;
    x *= norm;
    y *= norm; //will be 1 by def
    z *= norm;

    //standard D65 illuminant lin trans matrix 
    //will give [0-1] typically
    float r, g, b = 0;
    r =  3.2406*x - 1.5372*y - 0.4986*z;
    g = -0.9689*x + 1.8758*y + 0.0415*z;
    b =  0.0557*x - 0.2040*y + 1.0570*z;

    //clamp in case out of gamut
    r = constrain(r, 0.0, 1.0);
    g = constrain(g, 0.0, 1.0);
    b = constrain(b, 0.0, 1.0);

    rgb[0] = (uint8_t)(r*255.0);
    rgb[1] = (uint8_t)(g*255.0);
    rgb[2] = (uint8_t)(b*255.0);
}