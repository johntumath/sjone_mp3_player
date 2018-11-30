/*
 * LCDdisplay.cpp
 *
 *  Created on: Nov 9, 2018
 *      Author: bryan
 */

#include <LCDdisplay.h>
#include <stdio.h>

bool LCD_display::init()
{
    bool devicePresent = checkDeviceResponse();
    if(devicePresent)
    {
        writeReg('|','-');
    }
    return devicePresent;
}

bool LCD_display::write_char(char c)
{
    return write(c);
}

void LCD_display::set_splash(){
    writeReg('|',0x0A);
}

bool LCD_display::write_str(char* str, uint32_t length)
{
    /*
     * This function should work to send repeat starts to the i2c, but currently  doesn't work
     *///return write_str(str, length);
    for(int i=0; i<length; ++i) write_char(str[i]);
}

int LCD_display::position_cursor()
{
}
