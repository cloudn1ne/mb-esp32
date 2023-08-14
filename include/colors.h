#ifndef colors
#define colors
//
// colors_h
//
// KilterBoard default Color Codes
//
#define KB_STARTHOLD    0x00ff00        // green
#define KB_HANDHOLD     0x00ffff        // turquoise
#define KB_FOOTHOLD     0xffb600        // orange
#define KB_TOPHOLD      0xff00ff        // lilac

uint32_t KilterColorToRGB(uint8_t color);
uint8_t RGBToKilterColor(uint32_t rgb);

#endif