
#ifndef MP3_TASKS_H
#define MP3_TASKS_H

#include <stdio.h>
#include <string.h>

#include "LPC17xx.h"
#include "ff.h"


class VS1053{
private:
   char *songs[100];             //array of song titles
   int current_song_index;       //Initialize it at 0 with init function 
   int number_of_songs;          //Counting number of songs
public:
   VS1053();
   void getSongs();              //Grabs songs from lfname and writes into char *songs[100]
   char *getCurrentSong();       //Returns the current song that's being played
};

#endif