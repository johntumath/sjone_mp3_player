#include "tasks.hpp"
#include <stdio.h>
// #include "io.hpp"
#include "storage.hpp"
#include "gpio.hpp"
#include "mp3_tasks.hpp"

#include "utilities.h"
#include <string.h>
#include "event_groups.h"

//In chunks - data being moved in chunks of data
/*
    Read from SD card. Getting mp3 in chunks
*/
VS1053 mp3;

int main(void)
{
    //const uint32_t STACK_SIZE = 1024;

    mp3.getSongs();
    mp3.getCurrentSong();
    // xTaskCreate(xMp3Reader, "Read from SD Card", STACK_SIZE, (void*)1, 1, NULL);
    // xTaskCreate(xMp3Writer, "Write to MP3 Decoder", STACK_SIZE, (void*)1, 1, NULL);

    // scheduler_add_task(new terminalTask(PRIORITY_HIGH));
    // scheduler_start();
    return 0;
}
