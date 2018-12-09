#ifndef MP3INFO_H_
#define MP3INFO_H_

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include "ff.h"
#include <vector>
/*
  ID3 tag is 128 bytes long
*/

class MetaData {
private:
  std::vector<std::string> fileNames;
  int current_song_index;
  int number_of_songs;

public:
  MetaData();
  void getSongs();
  //char *getCurrentSong();
};

#endif /* MP3INFO_H */
