// colorswapper.h
#ifndef colorswapper_h
#define colorswapper_h

#include <Arduino.h>
#include "colors.h"

class ColorSwapper {

    private:    
      bool enabled;
      uint8_t startHoldColor,handHoldColor,footHoldColor,topHoldColor;      // replacement Kilter Color Codes
        
    public:        
        ColorSwapper(uint32_t startHoldRGB, uint32_t handHoldRGB, uint32_t footHoldRGB, uint32_t topHoldRGB);        
        ColorSwapper();
        void setStartHold(uint32_t rgb);
        void setHandHold(uint32_t rgb);
        void setFootHold(uint32_t rgb);
        void setToptHold(uint32_t rgb);
        void toggle(bool on_off);
        uint8_t swap(uint8_t color);
};


#endif