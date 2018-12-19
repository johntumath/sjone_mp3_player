/*
 * ViewController.h
 *
 *  Created on: Dec 17, 2018
 *      Author: John (Offline)
 */

#ifndef VIEWCONTROLLER_H_
#define VIEWCONTROLLER_H_

#include "LCDdisplay.h"
#include "Controller.h"
#include <string.h>

class ViewController {

public:
    ViewController(Controller*);
    void update_view(void);
    void shift_rows();
private:
    void transmit_to_LCD();
    Controller* control_ptr;
    LCD_display LCD;
    view_t current_view;
    std::string top_row_text;
    std::string bottom_row_text;
};

#endif /* VIEWCONTROLLER_H_ */
