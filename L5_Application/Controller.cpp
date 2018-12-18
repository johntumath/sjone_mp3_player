/*
 * Controller.cpp
 *
 *  Created on: Dec 16, 2018
 *      Author: John (Offline)
 */

#include <Controller.h>

Controller::Controller()
{
    // TODO Bryan might need to take in the pointer instead of creating new here.
    // (Heap vs Stack)
    handler = new MP3_Handler();
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
    return handler->get_buffer();
}

std::string Controller::get_current_artist()
{
    mp3_meta songinfo = handler->get_current_song();
    return songinfo.artist;
}

std::string Controller::get_current_album()
{
    mp3_meta songinfo = handler->get_current_song();
    return songinfo.album;
}

std::string Controller::get_current_track()
{
    mp3_meta songinfo = handler->get_current_song();
    return songinfo.song;
}

std::string Controller::get_menu_string()
{
}

int Controller::get_volume()
{
    return volume;
}

void Controller::on_click()
{
}
