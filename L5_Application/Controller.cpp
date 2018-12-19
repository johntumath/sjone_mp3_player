/*
 * Controller.cpp
 *
 *  Created on: Dec 16, 2018
 *      Author: John Tumath
 */

#include <Controller.h>

namespace{
    const int VOLUME_DELTA_VALUE=5;
    const int MAX_VOLUME = 100;
    const int MIN_VOLUME = 0;
}



Controller::Controller (SemaphoreHandle_t* sem_hold_in,
                        SemaphoreHandle_t* sem_view_update_in,
                        SemaphoreHandle_t* sem_start_playback_in)
{
    volume = 80;
    playlist = single_song;
    playing_song = false;
    pause = false;
    stop_playback = false;
    view_state = startup;
    sem_hold = sem_hold_in;
    sem_view_update = sem_view_update_in;
    sem_start_playback = sem_start_playback_in;
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

std::string Controller::get_text_to_display()
{
    return text_to_display;
}

int Controller::get_volume()
{
    return volume;
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
        case volume_menu:
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
   if (current_artist_list.empty())
   {
       current_artist_list = handler.get_artist_list();
       menu_artist_iterator = current_artist_list.begin();
       text_to_display = *menu_artist_iterator;
   }
   if (buttonStatus == (singlePressUp || doublePressUp))
   {
       //Update display with the previous artist in the vector
       if(menu_artist_iterator != current_artist_list.begin())
       {
           menu_artist_iterator--;
       }
       else
       {
           menu_artist_iterator = current_artist_list.end();
       }
       view_state = menu_artist;
       text_to_display = *menu_artist_iterator;
   }
   else if (buttonStatus == (singlePressDown || doublePressDown))
   {
       // Update display with the next artist in the vector
       if(menu_artist_iterator != current_artist_list.end())
       {
           menu_artist_iterator++;
       }
       else
       {
           menu_artist_iterator = current_artist_list.begin();
       }
       view_state = menu_artist;
       text_to_display = *menu_artist_iterator;
   }
   else if (buttonStatus == (singlePressRight || doublePressRight))
   {
       //Grab albums vector for selected artist, go to that menu
       current_album_list = handler.get_album_list(*menu_artist_iterator);
       menu_album_iterator = current_album_list.begin();
       view_state = menu_album;
   }
   else if (buttonStatus == (singlePressCenter || doublePressCenter))
   {
       //TODO Begin playback of all songs in the artist's vector
       //TODO TEMP Repeat right press
       current_album_list = handler.get_album_list(*menu_artist_iterator);
       menu_album_iterator = current_album_list.begin();
       view_state = menu_album;
   }
   //Left Click: Does nothing in this menu
}
void Controller::menu_album_click(buttonList buttonStatus)
{
   if (buttonStatus == (singlePressUp || doublePressUp))
   {
       // Update display with the previous album in the vector
       if(menu_album_iterator != current_album_list.begin())
       {
           menu_artist_iterator--;
       }
       else
       {
           menu_album_iterator = current_artist_list.end();
       }
       view_state = menu_album;
       text_to_display = *menu_album_iterator;
   }
   else if (buttonStatus == (singlePressDown || doublePressDown))
   {
       // Update display with the next artist in the vector
       if(menu_album_iterator != current_album_list.end())
       {
           menu_album_iterator++;
       }
       else
       {
           menu_album_iterator = current_album_list.begin();
       }
       view_state = menu_album;
       text_to_display = *menu_album_iterator;
   }
   else if (buttonStatus == (singlePressLeft || doublePressLeft))
   {
       // Go back to artist menu
       view_state = menu_artist;
       text_to_display = *menu_artist_iterator;
   }
   else if (buttonStatus == (singlePressRight || doublePressRight))
   {
       // Go into that album's track list
       current_songs_list = handler.get_song_list(*menu_artist_iterator ,*menu_album_iterator);
       menu_song_iterator = current_album_list.begin();
       view_state = menu_track;
       text_to_display = *menu_song_iterator;
   }
   else if (buttonStatus == (singlePressCenter || doublePressCenter))
   {
       //TODO Begin playback of all songs in the album's vector
       //TODO Currently just copy of press right
       current_songs_list = handler.get_song_list(*menu_artist_iterator ,*menu_album_iterator);
       menu_song_iterator = current_album_list.begin();
       view_state = menu_track;
       text_to_display = *menu_song_iterator;
   }

}
void Controller::menu_track_click(buttonList buttonStatus)
{
    if (buttonStatus == (singlePressUp || doublePressUp))
    {
        // Update menu display with the previous track in the vector
        if(menu_song_iterator != current_songs_list.begin())
        {
            menu_song_iterator--;
        }
        else
        {
            menu_song_iterator = current_songs_list.end();
        }
        view_state = menu_track;
        text_to_display = *menu_song_iterator;
    }
    else if (buttonStatus == (singlePressDown || doublePressDown))
    {
        //TODO Update display with the next track in the vector
        if(menu_song_iterator != current_songs_list.end())
        {
            menu_song_iterator++;
        }
        else
        {
            menu_song_iterator = current_songs_list.begin();
        }
        view_state = menu_track;
        text_to_display = *menu_song_iterator;
    }
    else if (buttonStatus == (singlePressLeft || doublePressLeft))
    {
        //TODO Go back to album menu
        view_state = menu_album;
        text_to_display = *menu_album_iterator;
    }
    else if (buttonStatus == (singlePressRight || doublePressRight|| singlePressCenter || doublePressCenter))
    {
        //TODO Play currently selected song
    }
}
void Controller::volume_click(buttonList buttonStatus)
{
    if (buttonStatus == (singlePressUp || doublePressUp))
    {
        volume += VOLUME_DELTA_VALUE;

        int new_volume = volume + VOLUME_DELTA_VALUE;
        if(new_volume < MAX_VOLUME)
            volume = new_volume;
        else
            volume = MAX_VOLUME;

        view_state = volume_menu;
        xSemaphoreGive(sem_view_update);

        //TODO Update Volume for MP3 Decoder

        //TODO Increase the volume by a certain percent and update display.
        // Wait a time to see if user pushes up/down again, then go back to play screen
    }
    else if (buttonStatus == (singlePressDown || doublePressDown))
    {
        int new_volume = volume - VOLUME_DELTA_VALUE;
        if(new_volume > MIN_VOLUME)
            volume = new_volume;
        else
            volume = MIN_VOLUME;

        view_state = volume_menu;
        xSemaphoreGive(sem_view_update);

        //TODO Update Volume for MP3 Decoder

        // TODO Decrease the volume by a certain percent and update display.
        // Wait a time to see if user pushes up/down again, then go back to play screen
    }
    else if (buttonStatus == (singlePressLeft || doublePressLeft))
    {
        //TODO Go back to play menu
    }
    else if (buttonStatus == (singlePressRight || doublePressRight))
    {
        //TODO Go back to play menu
    }
    else if (buttonStatus == (singlePressCenter || doublePressCenter))
    {
        //TODO Go back to play menu immediately
    }
}
void Controller::playing_click(buttonList buttonStatus)
{
    if (buttonStatus == (singlePressUp || doublePressUp))
    {
        volume += VOLUME_DELTA_VALUE;

        int new_volume = volume + VOLUME_DELTA_VALUE;
        if(new_volume < MAX_VOLUME)
            volume = new_volume;
        else
            volume = MAX_VOLUME;

        view_state = volume_menu;
        xSemaphoreGive(sem_view_update);

        //TODO Update Volume for MP3 Decoder
    }
    else if (buttonStatus == (singlePressDown || doublePressDown))
    {
        int new_volume = volume - VOLUME_DELTA_VALUE;
        if(new_volume > MIN_VOLUME)
            volume = new_volume;
        else
            volume = MIN_VOLUME;

        view_state = volume_menu;
        xSemaphoreGive(sem_view_update);

        //TODO Update Volume for MP3 Decoder
    }
    else if (buttonStatus == (singlePressLeft || doublePressLeft))
    {
        //TODO Stop playing song, go back to track menu.
    }
    else if (buttonStatus == (singlePressRight || doublePressRight))
    {
        // Go to next song
        if(current_song_iterator != current_songs_list.end() &&
                    ++current_song_iterator != current_songs_list.end())
        {
            struct mp3_meta current_song = handler.get_current_song();
            current_song.song = *current_song_iterator;
            handler.load_song(current_song);
            view_state = playing;
            text_to_display = *current_song_iterator;
            xSemaphoreGive(sem_start_playback);
            xSemaphoreGive(sem_view_update);
        }
    }
    else if (buttonStatus == (singlePressCenter || doublePressCenter))
    {
        // Pause play back
        pause = true;
        view_state = paused;
        text_to_display = *menu_song_iterator;
    }
}

bool Controller::end_of_song()
{
    return handler.end_of_song();
}

void Controller::pause_click(buttonList buttonStatus)
{
    if (buttonStatus == (singlePressUp || doublePressUp))
    {
        volume += VOLUME_DELTA_VALUE;

        int new_volume = volume + VOLUME_DELTA_VALUE;
        if(new_volume < MAX_VOLUME)
            volume = new_volume;
        else
            volume = MAX_VOLUME;

        view_state = volume_menu;
        xSemaphoreGive(sem_view_update);

        //TODO Update Volume for MP3 Decoder
    }
    else if (buttonStatus == (singlePressDown || doublePressDown))
    {
        int new_volume = volume - VOLUME_DELTA_VALUE;
        if(new_volume > MIN_VOLUME)
            volume = new_volume;
        else
            volume = MIN_VOLUME;

        view_state = volume_menu;
        xSemaphoreGive(sem_view_update);

        //TODO Update Volume for MP3 Decoder
    }
    else if (buttonStatus == (singlePressLeft || doublePressLeft))
    {
        //TODO Go back to play menu
    }
    else if (buttonStatus == (singlePressRight || doublePressRight))
    {
        //TODO Go back to play menu
    }
    else if (buttonStatus == (singlePressCenter || doublePressCenter))
    {
        //TODO Go back to play menu immediately
    }

}
void Controller::song_finished()
{
    if(current_song_iterator != current_songs_list.end() &&
            ++current_song_iterator != current_songs_list.end()){
        struct mp3_meta current_song = handler.get_current_song();
        current_song.song = *current_song_iterator;
        handler.load_song(current_song);
        xSemaphoreGive(sem_start_playback);
        xSemaphoreGive(sem_view_update);
    }
    else{
        //TODO Handle being at the end of the album
        view_state = menu_artist;
        xSemaphoreGive(sem_view_update);
    }
}
