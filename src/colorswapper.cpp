#include "colorswapper.h"

/// @brief optional constructor allows adjusting all colors
ColorSwapper::ColorSwapper(uint32_t startHoldRGB, uint32_t handHoldRGB, uint32_t footHoldRGB, uint32_t topHoldRGB)
{
     startHoldColor = RGBToKilterColor(startHoldRGB);
     handHoldColor = RGBToKilterColor(handHoldRGB);
     footHoldColor = RGBToKilterColor(footHoldRGB);
     topHoldColor = RGBToKilterColor(topHoldRGB);
}

/// @brief default constructor will use the KilterBoard colors
ColorSwapper::ColorSwapper()
{
     startHoldColor = RGBToKilterColor(KB_STARTHOLD);
     handHoldColor = RGBToKilterColor(KB_HANDHOLD);
     footHoldColor = RGBToKilterColor(KB_FOOTHOLD);
     topHoldColor = RGBToKilterColor(KB_TOPHOLD);
     Serial.printf("startHoldColor %06x = %d\n", KB_STARTHOLD, startHoldColor);
     Serial.printf("handHoldColor %06x = %d\n", KB_HANDHOLD, handHoldColor);
     Serial.printf("footHoldColor %06x = %d\n", KB_FOOTHOLD, footHoldColor);
     Serial.printf("topHoldColor %06x = %d\n", KB_TOPHOLD, topHoldColor);
}

/// @brief adjust individual hold category color
void ColorSwapper::setStartHold(uint32_t rgb)
{
    startHoldColor = RGBToKilterColor(rgb);
}
void ColorSwapper::setHandHold(uint32_t rgb)
{
    handHoldColor = RGBToKilterColor(rgb);
}
void ColorSwapper::setFootHold(uint32_t rgb)
{
    footHoldColor = RGBToKilterColor(rgb);
}
void ColorSwapper::setToptHold(uint32_t rgb)
{
    topHoldColor = RGBToKilterColor(rgb);
}


/// @brief swap colors as defined in object
/// @param color input kilter color code
/// @return  output kilter color code
uint8_t ColorSwapper::swap(uint8_t color)
{
    uint32_t rgb = KilterColorToRGB(color);

    if (rgb == KB_STARTHOLD)
	{
		return(startHoldColor);				
	}
    else if (rgb == KB_HANDHOLD)
    {
        return(handHoldColor);		
    }
    else if (rgb == KB_FOOTHOLD)
    {
        return(footHoldColor);		
    }
    else if (rgb == KB_TOPHOLD)
    {
        return(topHoldColor);		
    }
    else
	    return(color);
}