#include "tasks.hpp"
#include <LPC17xx.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <stdio.h>
#include "VS1053B.h"

VS1053B* mp3 = VS1053B::getInstance();

void print_mp3_status(void)
{
    VS1053b_status_t mp3status;
    mp3status.status = mp3->ReadSci(0x01);
    printf("STATUS: %x\n", mp3status.status);
    printf("SS_DO_NOT_JUMP: %d\n", mp3status.SSREFERENCESEL);
    printf("SSADCLOCK: %d\n", mp3status.SSADCLOCK);
    printf("SSAPDOWN1: %d\n", mp3status.SSAPDOWN1);
    printf("SSAPDOWN2: %d\n", mp3status.SSAPDOWN2);
    printf("SSVER: %d\n", mp3status.SSVER);
    printf("SSVCMDISABLE: %d\n", mp3status.SSVCMDISABLE);
    printf("SSVCMOVERLOAD: %d\n", mp3status.SSVCMOVERLOAD);
    printf("SSSWING: %d\n", mp3status.SSSWING);
    printf("SSDONOTJUMP: %d\n", mp3status.SSDONOTJUMP);
}

void PrintStatus(void * pvParameters)
{
    print_mp3_status();
    vTaskDelay(2000);
}

int main(void)
{
    mp3->init_VS1053B();
    const uint32_t STACK_SIZE_WORDS = 2048;
    xTaskCreate(PrintStatus, "MP3_Status", STACK_SIZE_WORDS, NULL, 1, NULL);
    scheduler_add_task(new terminalTask(PRIORITY_HIGH));
    scheduler_start();
    return -1;
}
