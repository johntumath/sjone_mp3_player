#include "tasks.hpp"
#include <stdio.h>
// #include "io.hpp"
#include "storage.hpp"
#include "gpio.hpp"

#include "utilities.h"
#include <string.h>
#include "event_groups.h"
//#include "mp3_tasks.hpp"


//In chunks - data being moved in chunks of data
/*
    Read from SD card. Getting mp3 in chunks
*/

//QueueHandle_t q;
//EventGroupHandle_t HandleEventGroup;

// VS1053 mp3;

const uint32_t BUFFER_SIZE = 16; 
char *songs[100];
int number_of_songs;

void getSongs(){
    DIR dir;
    FILINFO File_Info;
    FRESULT return_code;

    char file_name[128];

    f_opendir(&dir, "1:");
    
    while(1){
        File_Info.lfname = file_name; 
        File_Info.lfsize = sizeof(file_name);

        return_code = f_readdir(&dir, &File_Info);
        //Read directory contents
        if((FR_OK != return_code) || !File_Info.fname[0]){
            //should go here after first iteration
            printf("Do we go here?\n");
            break;
        }

        //If filename ends with .mp3, store song title in an array

        if(strstr(File_Info.lfname, ".mp3") || strstr(File_Info.lfname, ".MP3")){
            int length = strnlen(File_Info.lfname, 128);
            songs[number_of_songs] = new char[length + 1];
            strcpy(songs[number_of_songs], File_Info.lfname);
            number_of_songs++;
            printf("Number of songs: %i\n", number_of_songs);
        }
        
    }
    printf("Number of songs: %i\n", number_of_songs);
}


int main(void)
{
    const uint32_t STACK_SIZE = 1024;

    // FIL file;
    // f_open(&file, "1:song.mp3", FA_OPEN_EXISTING|FA_READ);

   // HandleEventGroup = xEventGroupCreate();
   // q = xQueueCreate(100, BUFFER_SIZE);

    getSongs();
    // xTaskCreate(xMp3Reader, "Read from SD Card", STACK_SIZE, (void*)1, 1, NULL);
    // xTaskCreate(xMp3Writer, "Write to MP3 Decoder", STACK_SIZE, (void*)1, 1, NULL);

    // scheduler_add_task(new terminalTask(PRIORITY_HIGH));
    // scheduler_start();
    return 0;
}
