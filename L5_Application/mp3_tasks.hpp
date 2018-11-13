#ifndef MP3_TASKS_H
#define MP3_TASKS_H

#include <stdio.h>
#include <string.h>

#include "LPC17xx.h"
#include "ff.h"


class VS1053{
private:
   char *songs[100]; //array of song titles
   int song_index;
   int number_of_songs = 0;
public:
   void getSongs();
};

#endif
