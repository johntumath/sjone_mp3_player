#include "tasks.hpp"
#include <LPC17xx.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <stdio.h>
#include "VS1053.h"

VS1053 MP3;
FATFS FatFs;
// GPIO exampleButton(P0_0);
// exampleButton.setAsInput();

void SineTest(void * pvParameters)
{
    MP3.init(P1_28, P1_29, P1_23);
    while(1)
    {
        MP3.startPlayMP3File("1:Test002.mp3");
    }

}

int main(void)
{
    //MP3.init(P1_28, P1_29, P1_23);
    MP3.getSongs();
    MP3.getCurrentSong();

    //const uint32_t STACK_SIZE_WORDS = 2048;
    //xTaskCreate(SineTest, "SineTest", STACK_SIZE_WORDS, NULL, PRIORITY_HIGH, NULL);
    //scheduler_add_task(new terminalTask(2));
    //scheduler_start();
    return -1;
}
