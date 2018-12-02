#ifndef MP3INFO_H_
#define MP3INFO_H_

#include <stdio.h>
#include <string.h>
#include "ff.h"

class MetaData {
private:
  char *songs[100];
  int current_song_index;
  int number_of_songs;

public:
  MetaData();
  void getSongs();
  char *getCurrentSong();
};

#endif /* MP3INFO_H */
