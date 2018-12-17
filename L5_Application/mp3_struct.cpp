#include "mp3_struct.h"


/*** Auxillary Functions ***/
uint32_t endian_swap(const uint32_t& value);
uint32_t decode_syncsafe(const uint32_t& value);
bool is_meta_end(std::string meta_type);
/*** Aux. Func. End ***/


MP3_Handler::MP3_Handler()
{
    //TODO:find each file and grab metadata for it
    //TODO:place file into song map using `songs[Artist][Album][Song]=filename;`
    //TODO:call load_song with first song on list to initialize current_track

}

//Private Fuctions
struct mp3_meta MP3_Handler::get_mp3_meta(std::string filename)
{
}

void MP3_Handler::remove_meta_head(struct mp3_track filename)
{
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
    //TODO return current song
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

// std::string MP3_Handler::get_file_name(std::string artist, std::string album, std::string song)
// {
//     //TODO: return name of file name associated with artist album song
// }

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

//Andrew's getSongs() Function. Going to leave it in case we need it
void MP3_Handler::getSongs(){
    DIR directory;
    static FILINFO fno;
    FRESULT res;
    char LF_name[128];
    fno.lfname = LF_name;
    fno.lfsize = sizeof(LF_name);

    // printf("asdf: %s\n", LF_name);
    res = f_opendir(&directory, "1:");

    if(res == FR_OK){
        while(1){
            res = f_readdir(&directory, &fno);

            if(res != FR_OK || fno.fname[0] == 0){
              break;
            }

            if((strstr(fno.fname, ".mp3") || strstr(fno.fname, ".MP3")) && (strstr(fno.fname, "~") == NULL)){
                if(strstr(fno.fname, "_")){
                  continue;
                }
                fileNames.push_back(fno.fname);
                std::cout<<"Songs in vector from fname: "<<number_of_songs<<": "<<fileNames[number_of_songs]<<"\n";
                number_of_songs++;
            }

            //If file name ends in .mp3 or .MP3 store in string array
            if(strstr(fno.lfname, ".mp3") || strstr(fno.lfname, ".MP3")){
                if(strstr(fno.lfname, "._")){
                  continue;
                }
                fileNames.push_back(fno.lfname);
                std::cout<<"Songs in vector from lfname: "<<number_of_songs<<": "<<fileNames[number_of_songs]<<"\n";
                number_of_songs++;
            }
        }
        // printf("Number of songs: %i\n", number_of_songs);
        f_closedir(&directory);
    }
}

std::string MP3_Handler::get_file_name()
{
    //TODO: return name of file name associated with artist album song

    std::string somestring = fileNames[current_song_index];
    std::cout<<somestring<<std::endl;
    return somestring;
}

void MP3_Handler::nextSong(){
  current_song_index++;
}
