/*
 * ViewController.cpp
 *
 *  Created on: Dec 17, 2018
 *      Author: John (Offline)
 */

#include <ViewController.h>
#include <iostream>

ViewController::ViewController(Controller* ctrl) : LCD(0xe4)
{
    top_row_text = "QUICK AND DIRTY";
    bottom_row_text = "MP3 PLAYER";

    control_ptr = ctrl;
    current_view = startup;

    transmit_to_LCD();
}

void ViewController::update_view(void)
{
    switch(control_ptr->get_view_state())
    {
        case startup:
            top_row_text = "QUICK AND DIRTY";
            bottom_row_text = "MP3 PLAYER";
            transmit_to_LCD();
            break;
        case menu_artist:
            top_row_text = "Artist:";
            bottom_row_text = control_ptr->get_text_to_display();
            transmit_to_LCD();
            break;
        case menu_album:
            top_row_text = "Album:";
            bottom_row_text = control_ptr->get_text_to_display();
            transmit_to_LCD();
            break;
        case menu_track:
            top_row_text = "Song:";
            bottom_row_text = control_ptr->get_text_to_display();
            transmit_to_LCD();
            break;
        case volume_menu:
            top_row_text = "Volume:";
            bottom_row_text = control_ptr->get_volume();
            transmit_to_LCD();
            break;
        case playing:
            top_row_text = "Playing: ";
            top_row_text = top_row_text + control_ptr->get_current_artist();
            bottom_row_text = control_ptr->get_current_track();
            transmit_to_LCD();
            break;
        case paused:
            top_row_text = "PAUSED";
            bottom_row_text = control_ptr->get_current_track();
            transmit_to_LCD();
            break;
        default:
            break;
    }
}

void ViewController::transmit_to_LCD()
{
    LCD.set_row_text(top_row,top_row_text);
    LCD.set_row_text(bottom_row,bottom_row_text);
}
