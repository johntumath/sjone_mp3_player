#include "FreeRTOS.h"
#include "handlers.hpp"
#include "tasks.hpp"
#include <LPC17xx.h>
#include <stdint.h>
#include <stdio.h>
#include "VS1053.h"
#include <queue.h>
#include <stdlib.h>
#include <string.h>
#include "storage.hpp"
#include "semphr.h"
#include "uart0_min.h"
#include "LabGPIOInterrupt.h"

VS1053 MP3;
char mp3FileName[32];
LabGpioInterrupts interrupt;
volatile bool paused, dreq_high;
QueueHandle_t mp3Bytes;
volatile SemaphoreHandle_t sem_start_reader, sem_start_player, sem_dreq_high;

volatile enum {playing, requeststop, stoptransmitting, playerstopped} player_state;

void Eint3Handler(void)
{
    interrupt.HandleInterrupt();
}

void DReqISR(void)
{
    long yield = 0;
    if (MP3.DREQ->read() == 1)
    {
        //uart0_puts("Inside ISR\n");
        xSemaphoreGiveFromISR(sem_dreq_high, &yield);
    }
    portYIELD_FROM_ISR(yield);
}

CMD_HANDLER_FUNC(volumeHandler)
{
    float vol= atoi(cmdParams.c_str());
    vol = 254 - 1 *(vol/100) - 100;
    MP3.setVolume((uint8_t)vol,(uint8_t)vol);
    printf("Volume set to %d\n", (uint8_t)vol );
    return true;
}

CMD_HANDLER_FUNC(pauseHandler)
{
    paused = !paused;
    return true;
}

CMD_HANDLER_FUNC(playHandler)
{
    char filename[32] = "1:";
    strcat(filename, cmdParams.c_str());
    strcpy(mp3FileName,filename);
    if (player_state != playerstopped)
    {
        player_state = requeststop;
        while (player_state != playerstopped)
        {
            vTaskDelay(10);
        }
    }
    paused = false;
    player_state = playing;
    xSemaphoreGive(sem_start_reader);
    xSemaphoreGive(sem_start_player);
    return true;
}

void Reader(void* pvParameters)
{
    FIL mp3File; //File descriptor for the file being read.
    unsigned char musicBlock[512]; //Local block of 512 bytes, used to move between reader and queue
    uint br; // Counts the number of bytes read during a read operation.
    while (1)
    {
        //Wait for signal to open file
        while(xSemaphoreTake(sem_start_reader, portMAX_DELAY)!= pdTRUE);
        //Open track for reading
        printf("Reader: Opening File\n");
        FRESULT res = f_open(&mp3File, mp3FileName, FA_READ);
        if (res != 0)
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
        f_read(&mp3File, musicBlock, 512, &br);
        do{
            if(br == 0 || (player_state == playerstopped)) //Empty file, break.
            {
              break;
            }
            else
            {
                if (!paused)
                {
                //Push music into Queue
                xQueueSend(mp3Bytes, &musicBlock, portMAX_DELAY);
                //Get next block of mp3 data
                f_read(&mp3File, musicBlock, 512, &br);
                }
            }
        } while (br != 0); // Loops while data still in file.
        // All done reading file, time to close file,
        // and signal completion of playback, and wait for new signal.
        f_close(&mp3File);
        printf("Reader: Closing file\n");
        player_state = playerstopped;
    }
}

void Player(void * pvParameters)
{
    unsigned char playerBuffer[512];
    u_int8 *bufP;

    while(1)
    {
        while(xSemaphoreTake(sem_start_player, portMAX_DELAY)!= pdTRUE);
        while(MP3.DREQ->read()==0)
        {
            vTaskDelay(2);
        }
        MP3.soft_reset();
        MP3.soft_reset();
        // reset playback
        MP3.sciWrite(SCI_MODE, SM_LINE1 | SM_SDINEW);
        // resync
        MP3.sciWrite(SCI_CLOCKF,0x6000);
        MP3.sciWrite(SCI_WRAMADDR, 0x1e29);
        MP3.sciWrite(SCI_WRAM, 0);
        MP3.sciWrite(SCI_DECODE_TIME, 0x00);
        MP3.sciWrite(SCI_DECODE_TIME, 0x00);

        while (player_state != playerstopped)
        {
            //Read off queue
            xQueueReceive(mp3Bytes, &playerBuffer, portMAX_DELAY);
            bufP = playerBuffer;
            //Begin playing the block received
            while(MP3.DREQ->read()==0)
            {
                vTaskDelay(2);
            }
            MP3._DCS->setLow();
            for (int i = 0; i < 512; i = i+32)
            {
                while(MP3.DREQ->read()==0)
                {
                    vTaskDelay(2);
                }
                MP3.spiwrite(bufP, 32);
                bufP += 32;
            }
            if (player_state == requeststop)
            {
                while(MP3.DREQ->read()==0)
                {
                    vTaskDelay(2);
                }
                MP3.sciWrite(SCI_MODE, SM_LINE1 | SM_SDINEW | SM_CANCEL);
                player_state = stoptransmitting;
            }
            else if (player_state == stoptransmitting)
            {
                while(MP3.DREQ->read()==0)
                {
                    vTaskDelay(2);
                }
                    player_state = playerstopped;
            }
            MP3._DCS->setHigh();
        }
    }
}

int main(void)
{
    scheduler_add_task(new terminalTask(PRIORITY_HIGH));
    MP3.init(P2_7, P1_29, P1_23);
    sem_start_reader = xSemaphoreCreateBinary();
    sem_start_player = xSemaphoreCreateBinary();
    sem_dreq_high = xSemaphoreCreateBinary();
    interrupt.Initialize();
    interrupt.AttachInterruptHandler(2,7,DReqISR,InterruptCondition::kRisingEdge);
    isr_register(EINT3_IRQn, Eint3Handler);
    player_state = playerstopped;
    paused = false;
    xSemaphoreGive(sem_dreq_high);
    mp3Bytes = xQueueCreate(2, 512);
    xTaskCreate(Reader, "Reader", STACK_BYTES(2096), NULL, 1, NULL);
    xTaskCreate(Player, "Player", STACK_BYTES(1048), NULL, 2, NULL);
    scheduler_start();
    return -1;
}
