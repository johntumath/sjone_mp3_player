#ifndef MP3INFO_H_
#define MP3INFO_H_

#include <stdio.h>
#include <map>
#include <vector>
#include <string.h>
#include "ff.h"

struct mp3_meta{
    std::string artist, song, album;
};

struct mp3_track{
    struct mp3_meta meta;
    FIL file;
};

class MP3_Handler {
private:
  struct mp3_track current_track;
  std::map <std::string, std::map<std::string, std::map<std::string, std::string>>> songs; //[Artist][Album][Song]

  struct mp3_meta get_mp3_meta(std::string filename);
  void remove_meta_head(struct mp3_track filename);
public:
  MP3_Handler();
  void load_song(std::string filename);
  void load_next_song();
  void load_prev_song();
  unsigned char * get_next_audio(uint32_t buffer_size);
  struct mp3_meta get_current_song();
  std::vector<std::string> get_artist_list();
  std::vector<std::string> get_album_list(std::string artist);
  std::vector<std::string> get_song_list(std::string artist, std::string album);
  std::string get_file_name(std::string artist, std::string album, std::string song);

};

#endif /* MP3INFO_H */
