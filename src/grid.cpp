#include "grid.h"


/// @brief create a KilterGrid mapping for a board
/// @param c number of columns
/// @param r number of rows
KilterGrid::KilterGrid(uint8_t c, uint8_t r)
{   
    uint8_t x,y;
    int16_t v;

    cols = c;
    rows = r;

    // create gridMapping array that stores the Kilterboard HoldNumbers in a X/Y array
    gridMapping = (int16_t*)malloc(c*r*2);
    memset(gridMapping, 0xFF, rows*cols*2);
    Serial.println();
    for (y=0; y < rows; y++)
    {        
        for (x=0; x < cols; x++)
        {
            if (y%2 == 0) {     // even row
                if (x % 2)      // even row + uneven column
                {
                    v = x*(rows/2)+x/2+y/2;
                }
                else
                  v = -1;
            }
            else                // odd row
            {                   
                if (x%2==0)     // odd row + even column
                {
                    v = (x+1)*(rows/2)+x/2-y/2-1;
                }
                else
                 v = -1;
            }           
            gridMapping[(cols*y)+x]=v;
        }          
    }          
}

/// @brief Dump KilterBoard layout on serial console
void KilterGrid::show()
{
  uint8_t x,y;
  int16_t v;

  Serial.println();
  Serial.printf("--------------------------------------------------------------------------------------\n");        
  for (y=0; y < rows; y++)
  {        
        for (x=0; x < cols; x++)
        {          
            v = gridMapping[(cols*y)+x];
            if (v != -1)
            {
                Serial.printf("(%3d) ", v);
            }
            else
            {
                Serial.printf("      ");
            }
        }       
        Serial.println();              
  }        
  Serial.printf("--------------------------------------------------------------------------------------\n\n");        
}


/// @brief Get Holder Numner from X/Y coordinates
/// @param x 
/// @param y 
int16_t KilterGrid::getHoldNumber(uint8_t x, uint8_t y)
{
    return gridMapping[(cols*y)+x];    
}


/// @brief Get X coordinate for holdNumber
/// @param holdNumber Kilterboard Hold number
int8_t KilterGrid::getX(uint16_t holdNumber)
{
    for (uint8_t y=0; y < rows; y++)
    {        
        for (uint8_t x=0; x < cols; x++)
        {          
            if (gridMapping[(cols*y)+x] == holdNumber) return (x);    
        }               
    }        
    return -1;
}

/// @brief Get X coordinate for holdNumber
/// @param holdNumber Kilterboard Hold number
int8_t KilterGrid::getY(uint16_t holdNumber)
{
    for (uint8_t y=0; y < rows; y++)
    {        
        for (uint8_t x=0; x < cols; x++)
        {          
            if (gridMapping[(cols*y)+x] == holdNumber) return (y);    
        }               
    }        
    return -1;
}