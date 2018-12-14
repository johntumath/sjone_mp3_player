/*
 * LCDdisplay.cpp
 *
 *  Created on: Nov 9, 2018
 *      Author: bryan
 */

#include <LCDdisplay.h>
#include <stdio.h>

std::string create_full_row_string(std::string row_text);

LCD_display::LCD_display(uint8_t address):i2c2_device(address)
{}

void LCD_display::set_row_text(enum display_row row, std::string text)
{
    if(row == top_row){
        top_row_text = std::move(text);
        top_row_iter = top_row_text.begin();
    }
    else if (row == bottom_row){
        bottom_row_text = std::move(text);
        bottom_row_iter = bottom_row_text.begin();
    }
    else
    {
        top_row_text = text;
        bottom_row_text = std::move(text);
        top_row_iter = top_row_text.begin();
        bottom_row_iter = bottom_row_text.begin();
    }
    refresh_screen();
}

void LCD_display::display_shift(enum display_row row)
{
    if(row == top_row){
        if(bottom_row_iter == bottom_row_text.end()){
            bottom_row_iter = bottom_row_text.begin();
        }
        else{
            ++bottom_row_iter;
        }
    }
    else if (row == bottom_row){
        if(top_row_iter == top_row_text.end()){
            top_row_iter = top_row_text.begin();
        }
        else{
            ++top_row_iter;
        }
    }
    else
    {
        display_shift(top_row);
        display_shift(bottom_row);
    }
    refresh_screen();
}

void LCD_display::reset_display_shift(enum display_row row)
{
    if(row == top_row){
        top_row_iter = top_row_text.begin();
    }
    else if (row == bottom_row){
        bottom_row_iter = bottom_row_text.begin();
    }
    else{
        top_row_iter = top_row_text.begin();
        bottom_row_iter = bottom_row_text.begin();
    }
    refresh_screen();
}

bool LCD_display::init()
{
    return checkDeviceResponse();
}

void LCD_display::refresh_screen(){
    position_cursor(0,0);
    write_str(create_full_row_string(std::string(bottom_row_iter, bottom_row_text.end())));

    position_cursor(1,0);
    write_str(create_full_row_string(std::string(top_row_iter, top_row_text.end())));
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

bool LCD_display::write_str(std::string str)
{
    return write_string((unsigned char *)str.c_str(), str.length());
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

std::string create_full_row_string(std::string row_text)
{
    if(row_text.length() >= ROW_WIDTH){
        return row_text.substr(0,ROW_WIDTH);
    }
    else{
        row_text.resize(ROW_WIDTH,' ');
        return row_text;
    }
}
