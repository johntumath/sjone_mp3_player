#include "mp3_struct.h"

MetaData::MetaData(){}

void MetaData::getSongs(){
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
            if(res != FR_OK || fno.fname[0] == 0) break;

            //If file name ends in .mp3 or .MP3 store in string array
            if(strstr(fno.lfname, ".mp3") || strstr(fno.lfname, ".MP3")){
                int length = strnlen(fno.lfname, 128);
                songs[number_of_songs] = new char[length + 1];
                strcpy(songs[number_of_songs], fno.lfname);
                printf("Song %i: %s\n", number_of_songs, songs[number_of_songs]);
                number_of_songs++;
            }
        }
        number_of_songs = number_of_songs/2; //All files generate .__file.ext. Divide by 2 to get an accurate count
        printf("Number of songs: %i\n", number_of_songs);
        f_closedir(&directory);
    }
}

char *MetaData::getCurrentSong(){
    printf("current Song: %s\n", songs[number_of_songs]);
    return songs[current_song_index];
}
