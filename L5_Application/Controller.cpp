/*
 * Controller.cpp
 *
 *  Created on: Dec 16, 2018
 *      Author: John Tumath
 */

#include <Controller.h>
#include <iostream>

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

    std::cout << "view state " << view_state << std::endl;
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
            view_state = startup;
            startup_click(buttonStatus);
            break;
    }
    xSemaphoreGive(*sem_view_update);
}

void Controller::startup_click(buttonList buttonStatus)
{
    std::cout << "Entering Startup Click" << std::endl;

    xSemaphoreGive(*sem_view_update);
    // TODO Get vector using get_artist_list()
    // TODO Add splash
    // TODO Set the iterator to the top
//    std::cout << handler.get_artist_list()[0] << std::endl;
    current_artist_list = handler.get_artist_list();
    menu_artist_iterator = current_artist_list.begin();
    text_to_display = *menu_artist_iterator;
    view_state = menu_artist;
    // TODO Set menu_string to select the first artist in the vector
    // menu_string =

}
void Controller::menu_artist_click(buttonList buttonStatus)
{
    std::cout << "Entering Menu - Artist " << std::endl;
   if (current_artist_list.empty())
   {
       current_artist_list = handler.get_artist_list();
       menu_artist_iterator = current_artist_list.begin();
       text_to_display = *menu_artist_iterator;
       view_state = menu_artist;
   }
   std::cout <<"AFTER MENU ARTIST LOAD\n"<< std::endl;
   if (buttonStatus == singlePressUp || buttonStatus == doublePressUp)
   {
       //Update display with the previous artist in the vector
       if(menu_artist_iterator != current_artist_list.begin())
       {
           menu_artist_iterator--;
       }
       else
       {
           menu_artist_iterator = current_artist_list.end()-1;
       }
       view_state = menu_artist;
       text_to_display = *menu_artist_iterator;
   }
   else if (buttonStatus == singlePressDown || buttonStatus == doublePressDown)
   {
       // Update display with the next artist in the vector
       if(menu_artist_iterator != current_artist_list.end()-1)
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
   else if (buttonStatus == singlePressRight || buttonStatus == doublePressRight)
   {
       //Grab albums vector for selected artist, go to that menu
       current_album_list = handler.get_album_list(*menu_artist_iterator);
       menu_album_iterator = current_album_list.begin();
       view_state = menu_album;
       text_to_display = *menu_album_iterator;
   }
   else if (buttonStatus == singlePressCenter || buttonStatus == doublePressCenter)
   {
       //TODO Begin playback of all songs in the artist's vector
       //TODO TEMP Repeat right press
       current_album_list = handler.get_album_list(*menu_artist_iterator);
       menu_album_iterator = current_album_list.begin();
       view_state = menu_album;
       text_to_display = *menu_album_iterator;
   }
   //Left Click: Does nothing in this menu
   //TODO Left click might bring us back to Song Playing Menu
}
void Controller::menu_album_click(buttonList buttonStatus)
{
    std::cout << "Entering Menu - Album " << std::endl;
   if (buttonStatus == singlePressUp || buttonStatus == doublePressUp)
   {
       // Update display with the previous album in the vector
       if(menu_album_iterator != current_album_list.begin())
       {
           menu_album_iterator--;
       }
       else
       {
           menu_album_iterator = current_artist_list.end()-1;
       }
       view_state = menu_album;
       text_to_display = *menu_album_iterator;
   }
   else if (buttonStatus == singlePressDown || buttonStatus == doublePressDown)
   {
       // Update display with the next artist in the vector
       if(menu_album_iterator != current_album_list.end()-1)
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
   else if (buttonStatus == singlePressLeft || buttonStatus == doublePressLeft)
   {
       // Go back to artist menu
       view_state = menu_artist;
       text_to_display = *menu_artist_iterator;
   }
   else if (buttonStatus == singlePressRight || buttonStatus == doublePressRight)
   {
       std::cout << "RP inside ALBUM CLICK\n" << std::endl;
       // Go into that album's track list
       std::cout << "Artist: " << *menu_artist_iterator << "Album: " << *menu_album_iterator << "\n" << std::endl;
       current_songs_list = handler.get_song_list(*menu_artist_iterator ,*menu_album_iterator);
       menu_song_iterator = current_songs_list.begin();
       view_state = menu_track;
       text_to_display = *menu_song_iterator;
   }
   else if (buttonStatus == singlePressCenter || buttonStatus == doublePressCenter)
   {
       std::cout << "CenterClk inside ALBUM CLICK\n" << std::endl;
       //TODO Begin playback of all songs in the album's vector
       //TODO Currently just copy of press right
       std::cout << "Artist: " << *menu_artist_iterator << "Album: " << *menu_album_iterator << "\n" << std::endl;
       current_songs_list = handler.get_song_list(*menu_artist_iterator ,*menu_album_iterator);
       menu_song_iterator = current_songs_list.begin();
       view_state = menu_track;
       text_to_display = *menu_song_iterator;
   }

}
void Controller::menu_track_click(buttonList buttonStatus)
{
    std::cout << "Entering Menu - Track\n" << std::endl;
    if (buttonStatus == singlePressUp || buttonStatus == doublePressUp)
    {
        // Update menu display with the previous track in the vector
        if(menu_song_iterator != current_songs_list.begin())
        {
            menu_song_iterator--;
        }
        else
        {
            menu_song_iterator = current_songs_list.end()-1;
        }
        view_state = menu_track;
        text_to_display = *menu_song_iterator;
    }
    else if (buttonStatus == singlePressDown || buttonStatus == doublePressDown)
    {
        //TODO Update display with the next track in the vector
        if(menu_song_iterator != current_songs_list.end()-1)
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
    else if (buttonStatus == singlePressLeft || buttonStatus == doublePressLeft)
    {
        //TODO Go back to album menu
        view_state = menu_album;
        text_to_display = *menu_album_iterator;
    }
    else if (buttonStatus == singlePressRight || buttonStatus == doublePressRight|| 
              buttonStatus == singlePressCenter || buttonStatus ==doublePressCenter)
    {

        struct mp3_meta current_song;
        current_song.artist = *menu_artist_iterator;
        current_song.album = *menu_album_iterator;
        current_song.song = *menu_song_iterator;

        current_artist_iterator = menu_artist_iterator;
        current_album_iterator = menu_album_iterator;
        current_song_iterator = menu_song_iterator;

        handler.load_song(current_song);
        view_state = playing;
        text_to_display = *current_song_iterator;
        std::cout << "Artist: " << *menu_artist_iterator << "Album: " << *menu_album_iterator << "Song: " << *menu_song_iterator << "\n" << std::endl;
        for (int i = 0; i < 10000000000; i++){};
        xSemaphoreGive(*sem_start_playback); //TODO Handle When song is currently playing !!!~!
        xSemaphoreGive(*sem_view_update);
    }
}
void Controller::volume_click(buttonList buttonStatus)
{
    std::cout << "Entering Menu - Volume\n" << std::endl;
    if (buttonStatus == singlePressUp || buttonStatus ==  doublePressUp)
    {
        volume += VOLUME_DELTA_VALUE;

        int new_volume = volume + VOLUME_DELTA_VALUE;
        if(new_volume < MAX_VOLUME)
            volume = new_volume;
        else
            volume = MAX_VOLUME;

        view_state = volume_menu;
        xSemaphoreGive(*sem_view_update);

        //TODO Update Volume for MP3 Decoder

        //TODO Increase the volume by a certain percent and update display.
        // Wait a time to see if user pushes up/down again, then go back to play screen
    }
    else if (buttonStatus == singlePressDown || buttonStatus ==  doublePressDown)
    {
        int new_volume = volume - VOLUME_DELTA_VALUE;
        if(new_volume > MIN_VOLUME)
            volume = new_volume;
        else
            volume = MIN_VOLUME;

        view_state = volume_menu;
        xSemaphoreGive(*sem_view_update);

        //TODO Update Volume for MP3 Decoder

        // TODO Decrease the volume by a certain percent and update display.
        // Wait a time to see if user pushes up/down again, then go back to play screen
    }
    else if (buttonStatus == singlePressLeft || buttonStatus ==  doublePressLeft)
    {
        view_state = playing;
        xSemaphoreGive(*sem_view_update);
        //TODO Go back to play menu
    }
    else if (buttonStatus == singlePressRight || buttonStatus ==  doublePressRight)
    {
        view_state = playing;
        xSemaphoreGive(*sem_view_update);
        //TODO Go back to play menu
    }
    else if (buttonStatus == singlePressCenter || buttonStatus ==  doublePressCenter)
    {
        view_state = playing;
        xSemaphoreGive(*sem_view_update);
        //TODO Go back to play menu immediately
    }
}
void Controller::playing_click(buttonList buttonStatus)
{
    std::cout << "Entering Playing_Click\n" << std::endl;
    if (buttonStatus == singlePressUp || buttonStatus ==  doublePressUp)
    {
        volume += VOLUME_DELTA_VALUE;

        int new_volume = volume + VOLUME_DELTA_VALUE;
        if(new_volume < MAX_VOLUME)
            volume = new_volume;
        else
            volume = MAX_VOLUME;

        view_state = volume_menu;
        xSemaphoreGive(*sem_view_update);

        //TODO Update Volume for MP3 Decoder
    }
    else if (buttonStatus == singlePressDown || buttonStatus ==  doublePressDown)
    {
        int new_volume = volume - VOLUME_DELTA_VALUE;
        if(new_volume > MIN_VOLUME)
            volume = new_volume;
        else
            volume = MIN_VOLUME;

        view_state = volume_menu;
        xSemaphoreGive(*sem_view_update);

        //TODO Update Volume for MP3 Decoder
    }
    else if (buttonStatus == singlePressLeft || buttonStatus ==  doublePressLeft)
    {
        //TODO Stop playing song, go back to track menu.
    }
    else if (buttonStatus == singlePressRight || buttonStatus ==  doublePressRight)
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
            xSemaphoreGive(*sem_start_playback);
            xSemaphoreGive(*sem_view_update);
        }
    }
    else if (buttonStatus == singlePressCenter || buttonStatus == doublePressCenter)
    {
        // Pause play back
        pause = true;
        view_state = paused;
        text_to_display = *menu_song_iterator;
    }
}

bool Controller::end_of_song()
{
    std::cout << "END OF SONG" << std::endl;
    return handler.end_of_song();
}

void Controller::pause_click(buttonList buttonStatus)
{
    std::cout << "Entering pause_Click " << std::endl;
    if (buttonStatus == singlePressUp ||buttonStatus ==  doublePressUp)
    {
        volume += VOLUME_DELTA_VALUE;

        int new_volume = volume + VOLUME_DELTA_VALUE;
        if(new_volume < MAX_VOLUME)
            volume = new_volume;
        else
            volume = MAX_VOLUME;

        view_state = volume_menu;
        xSemaphoreGive(*sem_view_update);

        //TODO Update Volume for MP3 Decoder
    }
    else if (buttonStatus == singlePressDown || buttonStatus == doublePressDown)
    {
        int new_volume = volume - VOLUME_DELTA_VALUE;
        if(new_volume > MIN_VOLUME)
            volume = new_volume;
        else
            volume = MIN_VOLUME;

        view_state = volume_menu;
        xSemaphoreGive(*sem_view_update);

        //TODO Update Volume for MP3 Decoder
    }
    else if (buttonStatus == singlePressLeft || buttonStatus == doublePressLeft)
    {
        //TODO Go back to play menu
    }
    else if (buttonStatus == singlePressRight || buttonStatus == doublePressRight)
    {
        //TODO Go back to play menu
    }
    else if (buttonStatus == singlePressCenter || buttonStatus == doublePressCenter)
    {
        //TODO Go back to play menu immediately
    }

}
void Controller::song_finished()
{
    std::cout << "SONG FINISHED" << std::endl;
    if(current_song_iterator != current_songs_list.end() &&
            ++current_song_iterator != current_songs_list.end()){
        struct mp3_meta current_song = handler.get_current_song();
        current_song.song = *current_song_iterator;
        handler.load_song(current_song);
        text_to_display = *current_song_iterator;
        xSemaphoreGive(*sem_start_playback);
        xSemaphoreGive(*sem_view_update);
    }
    else{
        //TODO Handle being at the end of the album
        view_state = menu_artist;
        xSemaphoreGive(*sem_view_update);
    }
}
