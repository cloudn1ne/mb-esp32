#include<Arduino.h>
#include "idleframe.h"

static uint8_t frame_num = 0;

void IdleFrame(KilterEncoder *tx_encoder)
{
    // default red X display	
    if (frame_num == 0)    //   X
    {
        tx_encoder->resetHolds();
        tx_encoder->setHold(164, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(251, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(167, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(254, RGBToKilterColor(0xFFFF00));

        tx_encoder->setHold(182, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(194, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(210, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(224, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(238, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(240, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(223, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(195, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(180, RGBToKilterColor(0xFF0000));
    }
    else if (frame_num == 1)    //   x
    {
        tx_encoder->resetHolds();
        tx_encoder->setHold(182, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(240, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(180, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(238, RGBToKilterColor(0xFFFF00));        

        tx_encoder->setHold(194, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(210, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(224, RGBToKilterColor(0xFF0000));        
        tx_encoder->setHold(223, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(195, RGBToKilterColor(0xFF0000));        
    }
    else if (frame_num == 2)    //   x (mini)
    {   
        tx_encoder->resetHolds();
        tx_encoder->setHold(194, RGBToKilterColor(0xFFFF00));        
        tx_encoder->setHold(224, RGBToKilterColor(0xFFFF00));        
        tx_encoder->setHold(223, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(195, RGBToKilterColor(0xFFFF00));

        tx_encoder->setHold(210, RGBToKilterColor(0xFF0000));        
    }
    else if (frame_num == 3)    //   . 
    {      
        tx_encoder->resetHolds();
        tx_encoder->setHold(210, RGBToKilterColor(0xFFFF00));        
    }
    else if (frame_num == 4)    //   + (mini)
    {   
        tx_encoder->resetHolds();
        tx_encoder->setHold(181, RGBToKilterColor(0xFFFF00));        
        tx_encoder->setHold(239, RGBToKilterColor(0xFFFF00));        
        tx_encoder->setHold(211, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(209, RGBToKilterColor(0xFFFF00));

        tx_encoder->setHold(210, RGBToKilterColor(0xFF0000));        
    }   
    else if (frame_num == 5) // + (big)
    {                
        tx_encoder->resetHolds();
        tx_encoder->setHold(212, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(208, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(152, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(268, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(211, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(210, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(209, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(239, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(181, RGBToKilterColor(0xFF0000));        
    }
    else if (frame_num == 6) // + (big)
    {        
        tx_encoder->resetHolds();
        tx_encoder->setHold(181, RGBToKilterColor(0xFFFF00));        
        tx_encoder->setHold(239, RGBToKilterColor(0xFFFF00));        
        tx_encoder->setHold(211, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(209, RGBToKilterColor(0xFFFF00));

        tx_encoder->setHold(210, RGBToKilterColor(0xFF0000));             
    }
    else if (frame_num == 7)    //   . 
    {      
        tx_encoder->resetHolds();
        tx_encoder->setHold(210, RGBToKilterColor(0xFFFF00));        
    }
    else if (frame_num == 8)    //   x (mini)
    {   
        tx_encoder->resetHolds();
        tx_encoder->setHold(194, RGBToKilterColor(0xFFFF00));        
        tx_encoder->setHold(224, RGBToKilterColor(0xFFFF00));        
        tx_encoder->setHold(223, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(195, RGBToKilterColor(0xFFFF00));

        tx_encoder->setHold(210, RGBToKilterColor(0xFF0000));        
    }
    else if (frame_num == 9)    //   x
    {        
        tx_encoder->resetHolds();
        tx_encoder->setHold(182, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(240, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(180, RGBToKilterColor(0xFFFF00));
        tx_encoder->setHold(238, RGBToKilterColor(0xFFFF00));        

        tx_encoder->setHold(194, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(210, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(224, RGBToKilterColor(0xFF0000));        
        tx_encoder->setHold(223, RGBToKilterColor(0xFF0000));
        tx_encoder->setHold(195, RGBToKilterColor(0xFF0000));        
    }
    else
    {
        frame_num = 0; // back to start
        return;
    }

    frame_num++;
}