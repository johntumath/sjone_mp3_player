#ifndef MP3INFO_H_
#define MP3INFO_H_

#include <stdio.h>
#include <map>
#include <vector>
#include <string.h>
#include "ff.h"

struct mp3_meta{
    std::string artist, album, song;
};

struct mp3_track{
    struct mp3_meta meta;
    FIL file;
    UINT bytes_read;
};

class MP3_Handler {
private:
  struct mp3_track current_track;
  std::map <std::string, std::map<std::string, std::map<std::string, std::string>>> songs; //[Artist][Album][Song]
  bool song_is_open;

  struct mp3_meta get_mp3_meta(std::string filename);
  void remove_meta_head();
public:
  unsigned char mp3bytes[512];

  MP3_Handler();
  void load_song(struct mp3_meta file_meta);
  void close_song();
  void load_next_song();
  void load_prev_song();
  bool end_of_song();
  void get_next_audio();
  unsigned char * get_buffer();
  struct mp3_meta get_current_song();
  std::vector<std::string> get_artist_list();
  std::vector<std::string> get_album_list(std::string artist);
  std::vector<std::string> get_song_list(std::string artist, std::string album);
  std::string get_file_name(struct mp3_meta);

};

#endif /* MP3INFO_H */
