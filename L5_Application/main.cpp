#include <LCDdisplay.h>
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
#include <string>
#include "mp3_struct.h"
#include "gpio.hpp"
#include "soft_timer.hpp"
#include "task.h"

LCD_display display(0xe4);
VS1053 MP3;
std::string mp3FileName;
LabGpioInterrupts interrupt;
volatile bool paused, newsong, next;
QueueHandle_t mp3Bytes;
SemaphoreHandle_t sem_start_reader, sem_dreq_high, sem_btn, sem_click, sem_held;
SoftTimer debouncer(200);

GPIO nextButton(P2_1);
GPIO playPauseButton(P2_2);
GPIO prevButton(P2_5);
GPIO volumeUpButton(P2_3);
GPIO volumeDownButton(P2_4);

enum buttonList{
  singlePressLeft,    //0
  doublePressLeft,    //1
  heldLeft,           //2
  singlePressCenter,  //3
  doublePressCenter,  //4
  heldCenter,         //5
  singlePressRight,   //6
  doublePressRight,   //7
  heldRight,          //8
  singlePressUp,      //9
  doublePressUp,      //10
  heldUp,             //11
  singlePressDown,    //12
  doublePressDown,    //13
  heldDown            //14
};

void shift_row(uint8_t row, uint8_t shift_amount, char* string, uint8_t len){
    display.position_cursor(row,0);
    char to_print [16];
    for(int i =0; i<16;++i){
        if(shift_amount < len){
            to_print[i] = string[shift_amount++];
        }
        else
        {
            to_print[i] = string[(shift_amount++)%len];
        }
    }
    display.write_str(std::string(to_print, 16));
}

void init_display(void*)
{
//    uart0_puts("setting rgb...");

    vTaskDelay(1000);
    char name[] = "Quick and Dirty MP3       ";
    int len =26;
    int shamt=0;
    vTaskDelay(1);
    display.init();
    uint32_t color =0;


    while(1){
        shift_row(0,shamt++,name,len);
        shamt = shamt % (len * 2 - 3);
        vTaskDelay(800);
        display.set_rgb(color&0xff, (color>>8)&0xff, (color>>16)&0xff);
        color++;
        printf("%i",color);
    }
}

void Eint3Handler(void)
{
    interrupt.HandleInterrupt();
}

void DReqISR(void)
{
    long yield = 0;
    if (MP3.DREQ->read() == 1)
    {
        xSemaphoreGiveFromISR(sem_dreq_high, &yield);
    }
    portYIELD_FROM_ISR(yield);
}

void ButtonPushISR()
{
  long yield = 0;
  xSemaphoreGiveFromISR(sem_btn, &yield);
  portYIELD_FROM_ISR(yield);
}

CMD_HANDLER_FUNC(volumeHandler)
{
    float vol= atoi(cmdParams.c_str());
    MP3.setVolume(vol);
    return true;
}

CMD_HANDLER_FUNC(pauseHandler)
{
    paused = !paused;
    return true;
}

void resetMP3()
{
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
}

void startPlay()
{
    paused = false;
    newsong = true;
    vTaskDelay(50);
    newsong = false;
    xSemaphoreGive(sem_start_reader);
}

CMD_HANDLER_FUNC(playHandler)
{
    char filename[32] = "1:";
    strcat(filename, cmdParams.c_str());
    mp3FileName = filename;
    startPlay();
    return true;
}

void PrintReadError(FRESULT res)
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

void Reader(void* pvParameters)
{
    FIL mp3File; //File descriptor for the file being read.
    unsigned char musicBlock[512]; //Local block of 512 bytes, used to move between reader and queue
    uint br; // Counts the number of bytes read during a read operation.
    while (1)
    {
        //Wait for signal to open file

        while(xSemaphoreTake(sem_start_reader, portMAX_DELAY)!= pdTRUE);
        bool isMP3File = (strstr(mp3FileName.c_str(), ".mp3") == NULL &&
        strstr(mp3FileName.c_str(), ".MP3") == NULL);
        //Open track for reading
        printf("Reader: Opening File: %s\n", mp3FileName.c_str());
        FRESULT res = f_open(&mp3File, mp3FileName.c_str(), FA_READ);
        if (res != 0 || isMP3File){
            PrintReadError(res);
            break;
        }
        res = f_read(&mp3File, musicBlock, 512, &br);
        if(res != 0){
            PrintReadError(res);
            break;
        }
        while (br != 0){
            if (newsong)
            {
                break;
            }
            else if (!paused)
            {
                //Push music into Queue
                xQueueSend(mp3Bytes, &musicBlock, portMAX_DELAY);
                //Get next block of mp3 data
                res = f_read(&mp3File, musicBlock, 512, &br);
                if(res != 0)
                {
                    PrintReadError(res);
                    break;
                }
            }
            else if (paused)
            {
                vTaskDelay(10);
            }
            else
            {
                break;
            }
        }
        // All done reading file, time to close file,
        // and signal completion of playback, and wait for new signal.
        f_close(&mp3File);
        printf("Reader: Closing file\n");
    }
}

void Player(void * pvParameters)
{
    unsigned char playerBuffer[512];
    u_int8 *bufP;
    while(1)
    {
        resetMP3();
        while (1)
        {
            //Read off queue
            xQueueReceive(mp3Bytes, &playerBuffer, portMAX_DELAY);
            bufP = playerBuffer;
            //Begin playing the block received
            if(MP3.DREQ->read()==0)
            {
                while(xSemaphoreTake(sem_dreq_high, portMAX_DELAY)!= pdTRUE);
            }
            MP3._DCS->setLow();
            for (int i = 0; i < 512; i = i+32)
            {
                if(MP3.DREQ->read()==0)
                {
                    while(xSemaphoreTake(sem_dreq_high, portMAX_DELAY)!= pdTRUE);
                }
                MP3.spiwrite(bufP, 32);
                bufP += 32;
            }
            MP3._DCS->setHigh();
        }
    }
}

void ButtonReaderTask(void * pvParameters)
{
  buttonList buttonStatus;

  nextButton.setAsInput();
  playPauseButton.setAsInput();
  prevButton.setAsInput();
  volumeUpButton.setAsInput();
  volumeDownButton.setAsInput();

  while(1)
  {
    while(xSemaphoreTake(sem_btn, portMAX_DELAY)!= pdTRUE);
    if(debouncer.expired())
    {
      if(prevButton.read() == 1)
      {
        buttonStatus = singlePressLeft;
        vTaskDelay(200);
        if(prevButton.read() == 1)
        {
          while(prevButton.read() == 1)
          {
            // printf("\nleft button is held\n");
            xSemaphoreGive(sem_held);
            vTaskDelay(50);
            buttonStatus = heldLeft;
          }
        }
        else
        {
          printf("\nleft button released\n");
          xSemaphoreGive(sem_click);
        }
      }
      else if(playPauseButton.read() == 1)
      {
        buttonStatus = singlePressCenter;
        vTaskDelay(200);
        if(playPauseButton.read() == 1)
        {
          while(playPauseButton.read() == 1)
          {
            // printf("\ncenter button is held\n");
            xSemaphoreGive(sem_held);
            vTaskDelay(50);
            buttonStatus = heldCenter;
          }
        }
        else
        {
          printf("\ncenter button released\n");
          xSemaphoreGive(sem_click);
        }
      }
      else if(nextButton.read() == 1)
      {
        buttonStatus = singlePressRight;
        vTaskDelay(200);
        if(nextButton.read() == 1)
        {
          while(nextButton.read() == 1)
          {
            // printf("\nright button is held\n");
            xSemaphoreGive(sem_held);
            vTaskDelay(50);
            buttonStatus = heldRight;
          }
        }
        else
        {
          printf("\nright button released\n");
          xSemaphoreGive(sem_click);
        }
      }
      else if(volumeUpButton.read() == 1)
      {
        buttonStatus = singlePressUp;
        vTaskDelay(200);
        if(volumeUpButton.read() == 1)
        {
          while(volumeUpButton.read() == 1)
          {
            // printf("\nup button is held\n");
            xSemaphoreGive(sem_held);
            vTaskDelay(50);
            buttonStatus = heldUp;
          }
        }
        else
        {
          printf("\nup button released\n");
          xSemaphoreGive(sem_click);
        }
      }
      else if(volumeDownButton.read() == 1)
      {
        buttonStatus = singlePressDown;
        vTaskDelay(200);
        if(volumeDownButton.read() == 1)
        {
          while(volumeDownButton.read() == 1)
          {
            // printf("\ndown button is held\n");
            xSemaphoreGive(sem_held);
            vTaskDelay(50);
            buttonStatus = heldDown;
          }
        }
        else
        {
          printf("\ndown button released\n");
          xSemaphoreGive(sem_click);
        }
      }
      vTaskDelay(50);
      debouncer.reset();
    }
  }
}

void TestTask(void * pvParameters)
{
  while(1)
  {
    if(xSemaphoreTake(sem_click, 0))
    {
      printf("\na button has been clicked\n");
    }
    else if(xSemaphoreTake(sem_held, 0))
    {
      printf("\na button is being held\n");
    }
  }
}

int main(void)
{
    scheduler_add_task(new terminalTask(3));
    MP3.init(P2_7, P1_29, P1_23);
    interrupt.Initialize();

    interrupt.AttachInterruptHandler(2,7,DReqISR,InterruptCondition::kRisingEdge);
    interrupt.AttachInterruptHandler(2,1, ButtonPushISR, InterruptCondition::kRisingEdge);
    interrupt.AttachInterruptHandler(2,2, ButtonPushISR, InterruptCondition::kRisingEdge);
    interrupt.AttachInterruptHandler(2,3, ButtonPushISR, InterruptCondition::kRisingEdge);
    interrupt.AttachInterruptHandler(2,4, ButtonPushISR, InterruptCondition::kRisingEdge);
    interrupt.AttachInterruptHandler(2,5, ButtonPushISR, InterruptCondition::kRisingEdge);
    isr_register(EINT3_IRQn, Eint3Handler);

    sem_start_reader = xSemaphoreCreateBinary();
    sem_dreq_high = xSemaphoreCreateBinary();
    sem_btn = xSemaphoreCreateBinary();
    sem_click = xSemaphoreCreateBinary();
    sem_held = xSemaphoreCreateBinary();
    mp3Bytes = xQueueCreate(2, 512);

    xTaskCreate(init_display, "Display", STACK_BYTES(2096), NULL, 2, NULL);
    xTaskCreate(Reader, "Reader", STACK_BYTES(2096), NULL, 1, NULL);
    xTaskCreate(Player, "Player", STACK_BYTES(1048), NULL, 2, NULL);
    xTaskCreate(ButtonReaderTask, "button", STACK_BYTES(1048), NULL, 2, NULL);
    xTaskCreate(TestTask, "sem", STACK_BYTES(1048), NULL, 2, NULL);
    scheduler_start();
    return -1;
}


// void startPlay()
// {
//   if(debouncer.expired())
//   {
//     if(doubleTap == 0)
//     {
//       char filename[32] = "1:";
//       strcat(filename, test.get_file_name().c_str());
//       mp3FileName = filename;
//       long yield = 0;
//       paused = false;
//       newsong = true;
//       vTaskDelay(50);
//       newsong = false;
//       xSemaphoreGiveFromISR(sem_start_reader, &yield);
//       uart0_puts("startPlay semaphore given\n");
//       portYIELD_FROM_ISR(yield);
//       doubleTap = 1;
//     }
//     else
//     {
//         paused = !paused;
//         printf("\nPause Status: %d\n", paused);
//     }
//     debouncer.reset();
//   }
// }
