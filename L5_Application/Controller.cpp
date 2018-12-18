/*
 * Controller.cpp
 *
 *  Created on: Dec 16, 2018
 *      Author: John Tumath
 */

#include <Controller.h>



Controller::Controller()
{
    volume = 80;
    // TODO Recieve the SemaphoreHandle_t pointers here
    // TODO Create an instance of MP3_Handler
    // TODO Set default states.
}

view_t Controller::get_view_state()
{
    return view_state;
}

bool Controller::is_paused()
{
    return pause;
}

bool Controller::is_playing_song()
{
    return playing_song;
}

bool Controller::is_stop_requested()
{
    return stop_playback;
}

unsigned char* Controller::get_next_block()
{
    handler.get_next_audio();
    return handler.get_buffer();
}

std::string Controller::get_current_artist()
{
    mp3_meta songinfo = handler.get_current_song();
    return songinfo.artist;
}

std::string Controller::get_current_album()
{
    mp3_meta songinfo = handler.get_current_song();
    return songinfo.album;
}

std::string Controller::get_current_track()
{
    mp3_meta songinfo = handler.get_current_song();
    return songinfo.song;
}

std::string Controller::get_menu_string()
{
}

int Controller::get_volume()
{
    return volume;
}

void Controller::toggle_pause()
{
}

void Controller::set_volume(int int1)
{
}


void Controller::on_click(buttonList buttonStatus)
{
    switch(view_state)
    {
        case startup:
            startup_click(buttonStatus);
            break;
        case menu_artist:
            menu_artist_click(buttonStatus);
            break;
        case menu_album:
            menu_album_click(buttonStatus);
            break;
        case menu_track:
            menu_track_click(buttonStatus);
            break;
        case volume:
            volume_click(buttonStatus);
            break;
        case playing:
            playing_click(buttonStatus);
            break;
        case paused:
            pause_click(buttonStatus);
            break;
        default:
            break;
    }
    xSemaphoreGive(sem_view_update);
}

void Controller::startup_click(buttonList buttonStatus)
{
    // TODO Get vector using get_artist_list()
    // Set the iterator to the top
    view_state = menu_artist;
    // Set menu_string to select the first artist in the vector
    // menu_string =

}
void Controller::menu_artist_click(buttonList buttonStatus)
{
   if (buttonStatus == (singlePressUp || doublePressUp))
   {
       //TODO Update display with the previous artist in the vector
   }
   else if (buttonStatus == (singlePressDown || doublePressDown))
   {
       //TODO Update display with the next artist in the vector

   }
   else if (buttonStatus == (singlePressRight || doublePressRight))
   {
       //TODO Grab albums vector for selected artist, go to that menu

   }
   else if (buttonStatus == (singlePressCenter || doublePressCenter))
   {

   }

   //Left Click: Does nothing in this menu

}
void Controller::menu_album_click(buttonList buttonStatus)
{

}
void Controller::menu_track_click(buttonList buttonStatus)
{

}
void Controller::volume_click(buttonList buttonStatus)
{

}
void Controller::playing_click(buttonList buttonStatus)
{

}
void Controller::pause_click(buttonList buttonStatus)
{

}
