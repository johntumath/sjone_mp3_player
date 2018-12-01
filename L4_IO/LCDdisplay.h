/*
 * LCDdisplay.h
 *
 *  Created on: Nov 9, 2018
 *      Author: bryan
 */

#include "i2c2_device.hpp"

#ifndef LCDDISPLAY_H_
#define LCDDISPLAY_H_

// LCD screen starts in normal mode and returns there when commands are done
//in normal mode:
//    - enter special setting mode: '|' or 0x7c or 124
//    - enter special command mode:        0xfe or 254
//    - perform a backspace:               0x08 or 8
//    - display new character to screen:   Any value that is not above value will work
//
//in special setting mode:
//    automatically returns to normal mode when done!
//    - change line width: this can be ignored
//    - send sw reset:            0x08 or 8
//    - toggle splash:            0x09 or 9
//    - save current as splash:   0x0a or 10
//    - set contrast:             0x18 or 24 then next byte gets saved as contrast
//    - set TWI address:          0x19 or 25 then next byte is new address
//    - create custom character:  0x1b - 0x22 or 27 - 34 chooses spot (limited number of spots) then will record new character unsure of details to do this
//    - display custom character: 0x23 - 0x2a or 35 - 42 choose which one to display
//    - set to RGB backlight mode:0x2b or 43  then 3 bytes for rgb
//    - Clear screen and buffer:  0x2d or 45
//    - print pipe character:     0x7c or 124
//    - change red, green, blue:  use base plus 8bit value divided by 8, base = {red:0x80, green:0x9e, blue:0xbc}
//in command mode:
//    - set cursor position: 0x80 | with 0x40 for second row | with 0x0* column position
//    - shift cursor or display: 0x10 | with 0x08 for display shift | 0x04 shift right
//    - set diplay cursor: 0x08 | with 0x04 display on | with 0x02 underline | with 0x01 blinking box // each option takes a bit position in the byte
//    - can send any command directly to lcd except 0x03 (8bit mode)
//
//    //transcribed from the code: https://github.com/sparkfun/OpenLCD/blob/master/firmware/OpenLCD/OpenLCD.ino
//

// OPEN LCD Commands

// Mode ops (Mode and Mode ops must be done in one transaction)
#define SETTING_MODE 0x7C
#define COMMAND_MODE 0xFE

//Setting Mode (SM) ops
#define SM_SW_RESET 0x08
#define SM_TOGGLE_SPLASH 0x09
#define SM_STORE_SPLASH 0x0A
#define SM_CLEAR_SCREEN 0x02D
#define SM_RED_BASE 0x80
#define SM_GREEN_BASE 0x9E
#define SM_BLUE_BASE 0xBC
#define SM_SET_CONTRAST 0x18
#define SM_SET_ADDRESS 0x19
#define SM_SET_RGB 0x2B

//Command Mode (CM) ops
#define CM_MOVE_CURSOR 0x80
#define CM_CURSOR_ROW 0x40
#define CM_CURSOR_COL 0x0F
#define CM_SHIFT_BASE 0x10
#define CM_SHIFT_DISP 0x08
#define CM_SHIFT_RIGHT 0x04
#define CM_SET_CURSOR 0x08
#define CM_CURS_DISP_ON 0x04
#define CM_CURS_UNDERLINE_ON 0x02 
#define CM_CURS_BLINK_BOX 0x01

// Fast lcd commands (can only be used indirectly in open lcd command mode see above comments )
// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
//#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00
#define LCD_Right 0
#define LCD_Left 1

class LCD_display : public i2c2_device {
public:
    LCD_display(uint8_t address):i2c2_device(address){};
    bool init();
    bool write_char(char c);
    bool write_str(char* str, uint32_t length);
    void clear_screen();
    void set_red(uint8_t value);
    void set_green(uint8_t value);
    void set_blue(uint8_t value);
    void set_splash();
    void toggle_splash();
    void reset_screen();
    int position_cursor(uint8_t row, uint8_t column);
private:
    void send_short_setting(uint8_t setting);
    void send_long_setting(uint8_t setting, uint8_t* options, uint8_t opt_length);
    void send_command(uint8_t command);
};

#endif /* LCDDISPLAY_H_ */
