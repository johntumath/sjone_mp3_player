/*
 * Controller.h
 *
 *  Created on: Dec 16, 2018
 *      Author: John (Offline)
 */

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include <string.h>
#include <map>
#include <mp3_struct.h>
#include "semphr.h"

enum view_t {startup, menu_artist, menu_album, menu_track, volume_menu, playing, paused};

enum buttonList{
  singlePressLeft,    //0
  doublePressLeft,    //1
  heldLeft,           //2
  singlePressCenter,  //3
  doublePressCenter,  //4
  heldCenter,         //5
  singlePressRight,   //6
  doublePressRight,   //7
  heldRight,          //8
  singlePressUp,      //9
  doublePressUp,      //10
  heldUp,             //11
  singlePressDown,    //12
  doublePressDown,    //13
  heldDown            //14
};

class Controller {
public:
    Controller(SemaphoreHandle_t*, SemaphoreHandle_t*, SemaphoreHandle_t*);
    //Viewer Functions
    view_t get_view_state();
    std::string get_text_to_display();
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
    void on_click(buttonList);
    void song_finished();
    bool end_of_song();
private:
    int volume;
    MP3_Handler handler;
    view_t view_state;
    std::string text_to_display;

    //    sem_hold = incoming signal from button task, alerting when button is being held
    //    sem_view_update = outgoing signal to alert the view task to refresh
    //    sem_start_playback = outgoing signal to start the reader task
    //    sem_song_ended = incoming signal from reader task, saying song is finished play back
    SemaphoreHandle_t* sem_hold, sem_view_update, sem_start_playback;

    enum {single_song, entire_album, all_songs_by_artist} playlist;
    bool playing_song, pause, stop_playback; //status flags
    std::vector<std::string>::iterator artist_iterator, album_iterator, song_iterator;
    std::vector<std::string> current_artist_list, current_album_list, current_songs_list;
    void startup_click(buttonList);
    void menu_artist_click(buttonList);
    void menu_album_click(buttonList);
    void menu_track_click(buttonList);
    void volume_click(buttonList);
    void playing_click(buttonList);
    void pause_click(buttonList);
};

#endif /* CONTROLLER_H_ */
