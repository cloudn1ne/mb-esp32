// grid.h
#ifndef grid_h
#define grid_h

#include <Arduino.h>

// 27x29 pixel grid for HomeWall 10x10
#define HOMEWALL_COLS 27
#define HOMEWALL_ROWS 29


class KilterGrid {
    private:            
        uint8_t cols,rows;
        int16_t *gridMapping;
        int16_t *grid;

    public:        
        KilterGrid(uint8_t cols, uint8_t rows);        
        int16_t getHoldNumber(uint8_t x, uint8_t y);
        int8_t getX(uint16_t holdNumber);
        int8_t getY(uint16_t holdNumber);
        void show();
};

#endif