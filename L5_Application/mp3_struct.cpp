#include "mp3_struct.h"
#include <iostream>


/*** Auxillary Functions ***/
uint32_t endian_swap(const uint32_t& value);
uint32_t decode_syncsafe(const uint32_t& value);
bool is_meta_end(std::string meta_type);
uint32_t mp3_get_length(std::string meta_length);
bool is_mp3(std::string filename);
/*** Aux. Func. End ***/


MP3_Handler::MP3_Handler()
{
    //TODO:call load_song with first song on list to initialize current_track

    DIR directory;
    static FILINFO file_info;
    FRESULT res;
    std::string LF_name;

    LF_name.resize(150);
    file_info.lfname = &LF_name[0];
    file_info.lfsize = LF_name.length();
    res = f_opendir(&directory, "1:");

    if(res == FR_OK){
        while(1){
            res = f_readdir(&directory, &file_info);
            if(res != FR_OK || file_info.fname[0] == 0) break;
            if(is_mp3(LF_name)){
                struct mp3_meta current_song = get_mp3_meta(LF_name);
                songs[current_song.artist][current_song.album][current_song.song]=LF_name;

                //Remove this print statement once debugging is done
                std::cout << "Song: " << current_song.song << " Artist: " << current_song.artist << " Album: " << current_song.artist << std::endl;
            }
        }
        f_closedir(&directory);
    }
}

//Private Fuctions
struct mp3_meta MP3_Handler::get_mp3_meta(std::string mp3_file)
{
    FIL mp3_finfo;
    struct mp3_meta meta_return;
    std::string  meta_head, frame_head, meta_body, frame_body;
    uint32_t meta_size, frame_size;
    uint bytes_read;

    FRESULT res = f_open(&mp3_finfo, mp3_file.c_str(), FA_READ);

    meta_head.resize(11);
    res = f_read(&mp3_finfo, static_cast<void*>(&meta_head[0]), 3, &bytes_read);

    meta_return.album="unknown album";
    meta_return.artist="unknown artist";
    meta_return.song= std::move(mp3_file);

    if(meta_head.substr(0,3) == "ID3"){
        bool all_meta_found=false, song_found=false, artist_found=false, album_found=false;
        std::string::iterator clear_buffer;

        res = f_read(&mp3_finfo, static_cast<void*>(&meta_head[3]), 7, &bytes_read);
        meta_size = mp3_get_length(meta_head.substr(3,4));

        meta_body.resize(meta_size);
        f_read(&mp3_finfo, static_cast<void*>(&meta_body[0]), meta_size, &bytes_read);

        do{
            frame_head = meta_body.substr(0,10);
            frame_size = mp3_get_length(frame_head.substr(4,4));
            frame_body.resize(frame_size);
            frame_body = meta_body.substr(10,frame_size);

            //clear buffer \0 from front of frame body so they are not mistaken for eof
            for(clear_buffer=frame_body.begin(); (*clear_buffer)== '\000'; ++clear_buffer);

            if(frame_head.substr(0,4) == "TIT2"){
                song_found = true;
                meta_return.song = std::string(clear_buffer,frame_body.end());
                if(artist_found && album_found) all_meta_found=true;
            }
            else if (frame_head.substr(0,4) == "TPE1"){
                artist_found =true;
                meta_return.artist = std::string(clear_buffer,frame_body.end());
                if(song_found && album_found) all_meta_found=true;
            }
            else if (frame_head.substr(0,4) == "TALB"){
                album_found = true;
                meta_return.album = std::string(clear_buffer,frame_body.end());
                if(song_found && artist_found) all_meta_found=true;
            }

            meta_body.erase(0,frame_size+10);
        }while(!meta_body.empty() && !is_meta_end(frame_head.substr(0,4)) && !all_meta_found);
    }
    else if(meta_head.substr(0,3) == "TAG"){
        //TODO: Handle version 1 Tag and Tag+ metadata headers
    }
    f_close(&mp3_finfo);
    return meta_return;
}

void MP3_Handler::remove_meta_head()
{
    std::string  meta_head;
    uint bytes_read;
    uint32_t meta_size;

    meta_head.resize(11);
    f_read(&current_track.file, static_cast<void*>(&meta_head[0]), 3, &bytes_read);

    if(meta_head.substr(0,3) == "ID3"){
        f_read(&current_track.file, static_cast<void*>(&meta_head[3]), 7, &bytes_read);
        meta_size = mp3_get_length(meta_head.substr(3,4));

        f_lseek(&current_track.file, meta_size+10);

    }
    else if(meta_head.substr(0,3) == "TAG"){
        f_lseek(&current_track.file, 128);
    }
    else{
        f_lseek(&current_track.file, 0);
    }
}

//Public Functions
void MP3_Handler::load_song(std::string filename)
{
    //TODO:close any open file
    //TODO:open file
    //TODO:call remove_meta_head
    //TODO:Load current_track with current info
}

void MP3_Handler::load_next_song()
{
    //TODO:close any open file
    //TODO:find next track in album if available f(probably use built in iterators and search for maps)
    //TODO:open file
    //TODO:Load current_track with current info
}

void MP3_Handler::load_prev_song()
{
    //TODO:close any open file
    //TODO:find next track in album if available f(probably use built in iterators and search for maps)
    //TODO:open file
    //TODO:Load current_track with current info
}

unsigned char* MP3_Handler::get_next_audio(uint32_t buffer_size)
{
    //I'm not sure if this is the correct signature for this function, but
    //TODO: put data into buffer
    //TODO: return buffer
}

struct mp3_meta MP3_Handler::get_current_song()
{
    return current_track.meta;
}

std::vector<std::string> MP3_Handler::get_artist_list()
{
    //TODO: Use Iterator in auto for loop to collect all artists into vector
    //TODO: return vector
}

std::vector<std::string> MP3_Handler::get_album_list(std::string artist)
{
    //TODO: Use Iterator in auto for loop to collect all albums into vector
    //TODO: return vector
}

std::vector<std::string> MP3_Handler::get_song_list(std::string artist, std::string album)
{
    //TODO: Use Iterator in auto for loop to collect all songs into vector
    //TODO: return vector
}

std::string MP3_Handler::get_file_name(std::string artist, std::string album, std::string song)
{
    //TODO: return name of file name associated with artist album song
}

/******** Auxillary Function Definitions ********/

uint32_t endian_swap(const uint32_t& value){
    uint32_t swapped_value = 0;
    swapped_value |= (value >> 24)& 0x000000ff;
    swapped_value |= (value << 24)& 0xff000000;
    swapped_value |= (value << 8) & 0x00ff0000;
    swapped_value |= (value >> 8) & 0x0000ff00;
    return swapped_value;
}

uint32_t decode_syncsafe(const uint32_t& value){
    return ((value>>3) & (0x7f<<21)) | ((value>>2) & (0x7f<< 14)) | ((value>>1) & (0x7f << 7)) | (value & 0x7f);
}

bool is_meta_end(std::string meta_type){
    if(meta_type.length() == 4) {
        return (uint32_t)meta_type[0] == 0 && (uint32_t)meta_type[1] == 0 &&
               (uint32_t)meta_type[2] == 0 && (uint32_t)meta_type[3] == 0;
    }
    else{
        return true;
    }
}

uint32_t mp3_get_length(std::string meta_length)
{
    return decode_syncsafe(endian_swap(*reinterpret_cast<const uint32_t *>(meta_length.c_str())));
}

bool is_mp3(std::string filename){
    std::string extension(filename.end()-3, filename.end());
    extension[0] = tolower(extension[0]);
    extension[1] = tolower(extension[1]);

    return extension == "mp3" ;
}
