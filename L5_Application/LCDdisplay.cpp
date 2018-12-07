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
    return checkDeviceResponse();
}

void LCD_display::send_short_setting(uint8_t setting)
{
    writeReg(SETTING_MODE, setting);
}

void LCD_display::send_long_setting(uint8_t setting, uint8_t* options, uint8_t opt_length)
{
    unsigned char * setting_opt = new unsigned char [opt_length + 1];
    setting_opt[0] = (unsigned char) setting;
    printf("%c\n",setting_opt[0]);
    for(int i=1; i < opt_length+1 ; ++i){
        setting_opt[i] = (unsigned char) options[i-1];
    }
    writeRegs(SETTING_MODE, setting_opt, opt_length+1);
    delete setting_opt;
}

void LCD_display::send_command(uint8_t command)
{
    writeReg(COMMAND_MODE, command);
}

bool LCD_display::write_char(char c)
{
    return write(c);
}

bool LCD_display::write_str(char* str, uint32_t length)
{

    return write_string((unsigned char *)str, length);
//    for(uint32_t i=0; i<length; ++i) write_char(str[i]);
    return true;
}

void LCD_display::clear_screen()
{
    send_short_setting(SM_CLEAR_SCREEN);
}

void LCD_display::set_red(uint8_t value)
{
    const uint8_t granularity=9;
    uint8_t scaled_value = value / granularity;
    send_short_setting(scaled_value + SM_RED_BASE);
}

void LCD_display::set_green(uint8_t value)
{
    const uint8_t granularity=9;
    uint8_t scaled_value = value / granularity;
    send_short_setting(scaled_value + SM_GREEN_BASE);
}

void LCD_display::set_blue(uint8_t value)
{
    const uint8_t granularity=9;
    uint8_t scaled_value = value / granularity;
    send_short_setting(scaled_value + SM_BLUE_BASE);
}

void LCD_display::set_splash()
{
    send_short_setting(SM_STORE_SPLASH);
}

void LCD_display::toggle_splash()
{
    send_short_setting(SM_TOGGLE_SPLASH);
}

void LCD_display::reset_screen()
{
    send_short_setting(SM_SW_RESET);
}

int LCD_display::position_cursor(uint8_t row, uint8_t column)
{
    if(row < 2 && column < 16){
        send_command(CM_MOVE_CURSOR | (CM_CURSOR_ROW & (row << 6)) | (CM_CURSOR_COL & column));
        return 1;
    }
    else{
        return -1;
    }
}

void LCD_display::set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
//    unsigned char rgb[3];
//    rgb[0]=red;
//    rgb[1]=green;
//    rgb[2]=blue;
//    send_long_setting(SM_SET_RGB, rgb , 3); //currently this command is not working with lcd
    set_red(red);
    set_green(green);
    set_blue(blue);
}
