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

VS1053 MP3;
char mp3FileName[32] = "1:TRACK320.mp3";
QueueHandle_t mp3Bytes;
SemaphoreHandle_t semplaysong;
volatile enum {playing, requeststop, stoptransmitting, playerstopped} player_state;


CMD_HANDLER_FUNC(volumeHandler)
{
    uint8_t vol= atoi(cmdParams.c_str());
    MP3.setVolume(vol,vol);
    printf("Volume set to %d\n", vol );
    return true;
}

CMD_HANDLER_FUNC(pauseHandler)
{
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
        while (player_state != playerstopped);
    }
    xSemaphoreGive(semplaysong);
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
        while(xSemaphoreTake(semplaysong, portMAX_DELAY)!= pdTRUE);
        //Open track for reading
        FRESULT res = f_open(&mp3File, mp3FileName, FA_READ);
        if (res != 0)
        {
            printf("Error opening file in reader task.");
            return;
        }
        f_read(&mp3File, musicBlock, 512, &br);
        do{
            if(br == 0) //Empty file, break.
            {
              break;
            }
            else
            {
                //Push music into Queue
                xQueueSend(mp3Bytes, &musicBlock, portMAX_DELAY);
                //Get next block of mp3 data
                f_read(&mp3File, musicBlock, 512, &br);
            }
        } while (br != 0); // Loops while data still in file.
        // All done reading file, time to close file,
        // and signal completion of playback, and wait for new signal.
        f_close(&mp3File);
    }
}

void Player(void * pvParameters)
{
    unsigned char playerBuffer[512];
    u_int8 *bufP;

    while(1)
    {
        MP3.soft_reset();
        // reset playback
        MP3.sciWrite(SCI_MODE, SM_LINE1 | SM_SDINEW);
        // resync
        MP3.sciWrite(SCI_CLOCKF,0x6000);
        MP3.sciWrite(SCI_WRAMADDR, 0x1e29);
        MP3.sciWrite(SCI_WRAM, 0);
        MP3.sciWrite(SCI_DECODE_TIME, 0x00);
        MP3.sciWrite(SCI_DECODE_TIME, 0x00);
        player_state = playing;
        while (MP3.DREQ->read()==0);
        while (player_state != playerstopped)
        {
            //Read off queue
            xQueueReceive(mp3Bytes, &playerBuffer, portMAX_DELAY);
            bufP = playerBuffer;
            //Begin playing the block received
            while (MP3.DREQ->read()==0);
            MP3._DCS->setLow();
            MP3.SPI0.setdivider(4);
            for (int i = 0; i < 512; i = i+32)
            {
                while (MP3.DREQ->read()==0);
                MP3.spiwrite(bufP, 32);
                bufP += 32;
            }
            MP3._DCS->setHigh();
            if (player_state == requeststop)
            {
                uint16_t temp = MP3.sciRead(SCI_MODE);
                MP3.sciWrite(SCI_MODE, temp | SM_CANCEL);
                player_state = stoptransmitting;
            }
            if (player_state == stoptransmitting)
            {
                uint16_t temp = MP3.sciRead(SCI_MODE);
                if (!(temp & SM_CANCEL))
                {
                    player_state = playerstopped;
                }
            }
        }

    }
}

int main(void)
{
    scheduler_add_task(new terminalTask(PRIORITY_HIGH));
    semplaysong = xSemaphoreCreateBinary();
    player_state = playerstopped;
    MP3.init(P1_28, P1_29, P1_23);
    mp3Bytes = xQueueCreate(2, 512);
    xTaskCreate(Reader, "Reader", STACK_BYTES(2096), NULL, 2, NULL);
    xTaskCreate(Player, "Player", STACK_BYTES(1048), NULL, 2, NULL);
    scheduler_start();
    return -1;
}
