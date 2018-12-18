/*
 * Controller.h
 *
 *  Created on: Dec 16, 2018
 *      Author: John (Offline)
 */

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include <string.h>
#include <mp3_struct.h>
#include "semphr.h"

enum view_t {startup, menu_artist, menu_album, menu_track, volume, playing, paused};

class Controller {
public:
    Controller();
    //Viewer Functions
    view_t get_view_state();
    std::string get_menu_string();
    int get_volume();
    //Reader Functions
    bool is_paused();
    bool is_playing_song();
    bool is_stop_requested();
    unsigned char* get_next_block();
    std::string get_current_artist();
    std::string get_current_album();
    std::string get_current_track();
    // Sem_click received in the controller task, perform functions 
    void on_click();
    //Console functions
    void toggle_pause();
    void set_volume(int);
private:
    int volume;
    MP3_Handler* handler;
    view_t view_state;
    SemaphoreHandle_t* sem_hold, sem_view_update, sem_start_playback;
    bool playing_song, pause, stop_playback; //status flags
};

#endif /* CONTROLLER_H_ */