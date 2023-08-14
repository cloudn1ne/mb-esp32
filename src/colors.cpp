#include<Arduino.h>
#include "colors.h"

/// @brief Kilter 8bit color to 24 RGB
/// @param color kilter color
/// @return RGB color
uint32_t KilterColorToRGB(uint8_t color)
{
    uint32_t rgbcode;

    uint8_t r = (int)(((color & 0b11100000) >> 5) / 7. * 255.);
    uint8_t g = (int)(((color & 0b00011100) >> 2) / 7. * 255.);
    uint8_t b = (int)(((color & 0b00000011) >> 0) / 3. * 255.);

    return((r << 16)|(g<<8)|b);
}

/// @brief 24 bit RGB to Kilter 8bit color
/// @param RGB color
/// @return color kilter color
uint8_t RGBToKilterColor(uint32_t rgb)
{
    uint8_t r = ((rgb >> 16) & 0xFF)/32.;
    uint8_t g = ((rgb >> 8) & 0xFF)/32.;
    uint8_t b = ((rgb) & 0xFF)/64.;
    return((r<<5)|(g<<2)|b);
}