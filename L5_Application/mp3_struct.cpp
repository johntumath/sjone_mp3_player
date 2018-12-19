#include "mp3_struct.h"
#include <iostream>
#include "tasks.hpp"

/*** Auxillary Functions ***/
uint32_t endian_swap(const uint32_t& value);
uint32_t decode_syncsafe(const uint32_t& value);
bool is_meta_end(std::string meta_type);
uint32_t mp3_get_length(std::string meta_length);
bool is_mp3(std::string filename);
void PrintFileReadError(FRESULT res);
std::string get_truncated_string(std::string extended_str);
std::string ucs2_to_string(std::string ucs_str);
/*** Aux. Func. End ***/

namespace{
    const int ID3V2_HEADER_SIZE = 10;
    const int ID3_TAG_META_SIZE = 30;
    const int ID3_TAG_DATA_SIZE = ID3_TAG_META_SIZE * 3; //(3): Artist, Album, Song
    const int ID3_EX_TAG_META_SIZE = 60;
    const int ID3_EX_TAG_DATA_SIZE = ID3_EX_TAG_META_SIZE * 3; //(3): Artist, Album, Song
}


MP3_Handler::MP3_Handler()
{
    //TODO:call load_song with first song on list to initialize current_track
    std::cout << "ENTERING MP3_HANDLER CONSTRUCTOR" << std::endl;
    song_is_open = false;
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
            if(res != FR_OK || file_info.fname[0] == 0) {
                if (res != FR_OK) PrintFileReadError(res);
                break;
            }
            if(is_mp3(LF_name)){
                struct mp3_meta current_song = get_mp3_meta(LF_name);
                size_t end = LF_name.find('\0');
                songs[current_song.artist][current_song.album][current_song.song]=LF_name.substr(0,end);
                //Remove this print statement once debugging is done
                std::cout << "Song: " << current_song.song << " Artist: " << current_song.artist << " Album: " << current_song.album << std::endl;
            }
            LF_name.resize(150);
        }
        f_closedir(&directory);
    }
}

//Private Fuctions
struct mp3_meta MP3_Handler::get_mp3_meta(std::string mp3_file)
{
    FIL mp3_finfo;
    struct mp3_meta meta_return;
    std::string  meta_head, frame_head, meta_body, frame_body, frame_type;
    std::wstring wframe_body;
    uint32_t meta_size, frame_size;
    uint bytes_read;

    /* Set default values for missing meta */
    meta_return.album="Unknown Album";
    meta_return.artist="Unknown Artist";
    meta_return.song = std::string(mp3_file.begin(), mp3_file.begin() + mp3_file.find('\0'));

    /* Add Directory and open file */
    mp3_file = "1:" + mp3_file;
    FRESULT res = f_open(&mp3_finfo, mp3_file.c_str(), FA_READ);
    if (res != 0){
        PrintFileReadError (res);
        std::cout << "Filename was: " << mp3_file << std::endl;
    }

    meta_head.resize(ID3V2_HEADER_SIZE);
    res = f_read(&mp3_finfo, static_cast<void*>(&meta_head[0]), ID3V2_HEADER_SIZE, &bytes_read);

    if(meta_head.substr(0,3) == "ID3"){
        uint8_t text_type;
        bool all_meta_found=false, song_found=false, artist_found=false, album_found=false;
        std::string::iterator clear_buffer;
        meta_size = mp3_get_length(meta_head.substr(6,9));
        frame_head.resize(ID3V2_HEADER_SIZE);

        while(!all_meta_found && mp3_finfo.fptr < meta_size + 10){
            res = f_read(&mp3_finfo, static_cast<void*>(&frame_head[0]), ID3V2_HEADER_SIZE, &bytes_read);
            frame_size = mp3_get_length(frame_head.substr(4,7));
            frame_type = frame_head.substr(0,4);
            if(frame_head.substr(0,4) == "TIT2"){
                res = f_read(&mp3_finfo, static_cast<void*>(&text_type), 1, &bytes_read);
                if(text_type == 0){
                    meta_return.song.resize(frame_size);
                    res = f_read(&mp3_finfo, static_cast<void*>(&meta_return.song[0]), frame_size-1, &bytes_read);
                }
                else if(text_type == 1){
                    frame_body.resize(frame_size-1);
                    res = f_read(&mp3_finfo, static_cast<void*>(&frame_body[0]), frame_size-1, &bytes_read);
                    meta_return.song = ucs2_to_string(frame_body);
                }
                song_found = true;
                if(artist_found && album_found) all_meta_found=true;
            }
            else if (frame_head.substr(0,4) == "TPE1"){
                res = f_read(&mp3_finfo, static_cast<void*>(&text_type), 1, &bytes_read);
                if(text_type == 0){
                    meta_return.artist.resize(frame_size);
                    res = f_read(&mp3_finfo, static_cast<void*>(&meta_return.artist[0]), frame_size-1, &bytes_read);
                }
                else if(text_type == 1){
                    frame_body.resize(frame_size-1);
                    res = f_read(&mp3_finfo, static_cast<void*>(&frame_body[0]), frame_size-1, &bytes_read);
                    meta_return.artist = ucs2_to_string(frame_body);
                }
                artist_found =true;
                if(song_found && album_found) all_meta_found=true;
            }
            else if (frame_head.substr(0,4) == "TALB"){
                res = f_read(&mp3_finfo, static_cast<void*>(&text_type), 1, &bytes_read);
                if(text_type == 0){
                    meta_return.album.resize(frame_size);
                    res = f_read(&mp3_finfo, static_cast<void*>(&meta_return.album[0]), frame_size-1, &bytes_read);
                }
                else if(text_type == 1){
                    frame_body.resize(frame_size-1);
                    res = f_read(&mp3_finfo, static_cast<void*>(&frame_body[0]), frame_size-1, &bytes_read);
                    meta_return.album = ucs2_to_string(frame_body);
                }
                album_found = true;
                if(song_found && artist_found) all_meta_found=true;
            }
            else{
                /* Skip Unwanted Frame */
                if(*(uint32_t*)frame_head.substr(0,4).c_str()==0){
                    f_lseek(&mp3_finfo, meta_size + 10);
                }
                else {
                    f_lseek(&mp3_finfo, mp3_finfo.fptr + frame_size);
                }
            }
        }
    }
    else{
        f_lseek(&mp3_finfo, mp3_finfo.fsize-128);
        meta_body.resize(95);
        f_read(&mp3_finfo, static_cast<void*>(&meta_body[0]), 93, &bytes_read);
        if(meta_body.substr(0,3)=="TAG"){
            meta_return.song = get_truncated_string(meta_body.substr(3,30));
            meta_return.artist = get_truncated_string(meta_body.substr(33,30));
            meta_return.album = get_truncated_string(meta_body.substr(63,30));
        }
        else{
            f_lseek(&mp3_finfo, mp3_finfo.fsize-227);
            meta_body.resize(190);
            f_read(&mp3_finfo, static_cast<void*>(&meta_body[0]), 184, &bytes_read);
            if(meta_body.substr(0,4)=="TAG+"){
                meta_return.song = get_truncated_string(meta_body.substr(4,60));
                meta_return.artist = get_truncated_string(meta_body.substr(64,30));
                meta_return.album = get_truncated_string(meta_body.substr(124,30));
            }
        }

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
void MP3_Handler::load_song(struct mp3_meta file_meta)
{
    std::string file_string("1:");
    file_string += get_file_name(file_meta);
    std::cout << "LOAD SONG FILE META: " << file_string << std::endl;
    close_song();
    vTaskDelay(100);
    std::cout << "After Close Song\n " << file_string << std::endl;
    f_open(&current_track.file, file_string.c_str(), FA_READ );
    current_track.meta.artist = file_meta.artist;
    current_track.meta.album = file_meta.album;
    current_track.meta.song = file_meta.song;
    song_is_open = true;
}

void MP3_Handler::close_song()
{
    if(song_is_open){   
        f_close(&current_track.file);
        song_is_open = false;
    }
}

void MP3_Handler::load_next_song()
{
    std::map<std::string, std::string>::iterator next_song;
    next_song = ++(songs[current_track.meta.artist][current_track.meta.album].find(current_track.meta.song));
    if(next_song != songs[current_track.meta.artist][current_track.meta.album].end()){
        current_track.meta.song = next_song->first;
        load_song(current_track.meta);
    }
    else{
        // TODO: Handle requesting next song if on first song on album
    }
}

void MP3_Handler::load_prev_song()
{
    std::map<std::string, std::string>::iterator prev_song;
    prev_song = --songs[current_track.meta.artist][current_track.meta.album].find(current_track.meta.song);
    if(prev_song != songs[current_track.meta.artist][current_track.meta.album].begin()
        && prev_song != songs[current_track.meta.artist][current_track.meta.album].end()){
        current_track.meta.song = prev_song->first;
        load_song(current_track.meta);
    }
    else{
        // TODO: Handle requesting previous song if on first song on album (prev_song==begin())
        // TODO: Handle song not found (prev_song==end())
    }
}

void MP3_Handler::get_next_audio()
{
    FRESULT res = f_read(&current_track.file, mp3bytes, 512, &current_track.bytes_read);
    if(res != 0) PrintFileReadError(res);
}

bool MP3_Handler::end_of_song()
{
    return current_track.bytes_read == 0;
}
unsigned char * MP3_Handler::get_buffer()
{
    return mp3bytes;
}

struct mp3_meta MP3_Handler::get_current_song()
{
    return current_track.meta;
}

std::vector<std::string> MP3_Handler::get_artist_list()
{
    std::vector <std::string> artist_list;
    for (auto& t : songs) artist_list.push_back(t.first);

    return artist_list;
}

std::vector<std::string> MP3_Handler::get_album_list(std::string artist)
{
    std::vector <std::string> album_list;
    for (auto& t : songs[artist]) album_list.push_back(t.first);

    return album_list;
}

std::vector<std::string> MP3_Handler::get_song_list(std::string artist, std::string album)
{
    std::vector <std::string> song_list;
    for (auto& t : songs[artist][album]) song_list.push_back(t.first);

    return song_list;
}

std::string MP3_Handler::get_file_name(struct mp3_meta mp3)
{
    return songs[mp3.artist][mp3.album][mp3.song];
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
    return (value>>3)& (0x7f<<21)|(value>>2)& (0x7f<< 14)|(value>>1)& (0x7f << 7)|value & 0x7f;
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
    size_t end = filename.find('\0');
    std::string extension((filename.begin()+ (end-3)), (filename.begin()+end));
    extension[0] = tolower(extension[0]);
    extension[1] = tolower(extension[1]);
    return extension == "mp3" ;
}

std::string ucs2_to_string(std::string ucs_str){
    std::string endianness = ucs_str.substr(0,2);
    std::string return_str;

    return_str.resize(ucs_str.size()/2);

//    return_str += ucs_str[0]>>16;
    if(*(uint16_t*)(endianness.c_str()) == 0xFFFE){
        std::cout<< "big endian" << std::endl;
        for(uint16_t i=2; i < ucs_str.size() ; i+=1){
            if(i%2==1)
                return_str += ucs_str[i];
        }
        //TODO: handle in this endian
    }
    else if (*(uint16_t*)(endianness.c_str()) == 0xFEFF){
        std::cout<< "little endian" << std::endl;
        for(uint16_t i=2; i < ucs_str.size() ; i+=1){
            if(i%2==0)
                return_str += ucs_str[i];
        }
    }
    else{
        std::cout<< "???? endian    " << endianness << std::endl;
    }
    return return_str;
}

std::string get_truncated_string(std::string extended_str){
    size_t end = extended_str.find('\0');
    std::string truncated_string(extended_str.begin(), extended_str.begin() + end);
    return truncated_string;
}


void PrintFileReadError(FRESULT res)
{
    switch(res)
    {
        case 1: uart0_puts("Read Error: FR_DISK_ERR"); break;
        case 2: uart0_puts("Read Error: FR_INT_ERR"); break;
        case 3: uart0_puts("Read Error: FR_NOT_READY"); break;
        case 4: uart0_puts("Read Error: FR_NO_FILE"); break;
        case 5: uart0_puts("Read Error: FR_NO_PATH"); break;
        case 6: uart0_puts("Read Error: FR_INVALID_NAME"); break;
        case 7: uart0_puts("Read Error: FR_DENIED"); break;
        case 8: uart0_puts("Read Error: FR_EXIST"); break;
        case 9: uart0_puts("Read Error: FR_INVALID_OBJECT"); break;
        case 10: uart0_puts("Read Error: FR_WRITE_PROTECTED"); break;
        case 11: uart0_puts("Read Error: FR_INVALID_DRIVE"); break;
        case 12: uart0_puts("Read Error: FR_NOT_ENABLED"); break;
        case 13: uart0_puts("Read Error: FR_NO_FILESYSTEM"); break;
        case 14: uart0_puts("Read Error: FR_MKFS_ABORTED"); break;
        case 15: uart0_puts("Read Error: FR_TIMEOUT"); break;
        case 16: uart0_puts("Read Error: FR_LOCKED"); break;
        case 17: uart0_puts("Read Error: FR_NOT_ENOUGH_CORE"); break;
        case 18: uart0_puts("Read Error: FR_TOO_MANY_OPEN_FILES"); break;
        case 19: uart0_puts("Read Error: FR_INVALID_PARAMETER"); break;
        default : uart0_puts("Read Error: Unknown"); break;
        return;
    }
}
