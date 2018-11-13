#include "mp3_tasks.hpp"

void VS1053::getSongs(){
    DIR directory; 
    static FILINFO fno;
    FRESULT res;
    char LF_name[128];
    fno.lfname = LF_name;
    fno.lfsize = sizeof(LF_name);

    res = f_opendir(&directory, "1:");
    if(res == FR_OK){
        while(1){
            res = f_readdir(&directory, &fno);
            if(res != FR_OK || fno.fname[0] == 0) break;
            
            if(strstr(fno.lfname, ".mp3") || strstr(fno.lfname, ".MP3")){
                int length = strnlen(fno.lfname, 128);
                //printf("fno.lfname: %s\n", fno.lfname);
                songs[song_index] = new char[length + 1];
                strcpy(songs[song_index], fno.lfname);
                printf("Songs: %s\n", *songs);
                number_of_songs++;
            }
        }
        number_of_songs = number_of_songs / 2; //All files generate .__file.ext. Divide by 2 to get an accurate count
        printf("Number of songs: %i\n", number_of_songs);
        f_closedir(&directory);
    }
}
