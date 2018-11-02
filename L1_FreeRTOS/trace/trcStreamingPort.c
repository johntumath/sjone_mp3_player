/*******************************************************************************
 * Trace Recorder Library for Tracealyzer v3.2.0
 * Percepio AB, www.percepio.com
 *
 * trcStreamingPort.c
 *
 * Supporting functions for trace streaming, used by the "stream ports" 
 * for reading and writing data to the interface.
 * Existing ports can easily be modified to fit another setup, e.g., a 
 * different TCP/IP stack, or to define your own stream port.
 *
  * Terms of Use
 * This file is part of the trace recorder library (RECORDER), which is the 
 * intellectual property of Percepio AB (PERCEPIO) and provided under a
 * license as follows.
 * The RECORDER may be used free of charge for the purpose of recording data
 * intended for analysis in PERCEPIO products. It may not be used or modified
 * for other purposes without explicit permission from PERCEPIO.
 * You may distribute the RECORDER in its original source code form, assuming
 * this text (terms of use, disclaimer, copyright notice) is unchanged. You are
 * allowed to distribute the RECORDER with minor modifications intended for
 * configuration or porting of the RECORDER, e.g., to allow using it on a 
 * specific processor, processor family or with a specific communication
 * interface. Any such modifications should be documented directly below
 * this comment block.  
 *
 * Disclaimer
 * The RECORDER is being delivered to you AS IS and PERCEPIO makes no warranty
 * as to its use or performance. PERCEPIO does not and cannot warrant the 
 * performance or results you may obtain by using the RECORDER or documentation.
 * PERCEPIO make no warranties, express or implied, as to noninfringement of
 * third party rights, merchantability, or fitness for any particular purpose.
 * In no event will PERCEPIO, its technology partners, or distributors be liable
 * to you for any consequential, incidental or special damages, including any
 * lost profits or lost savings, even if a representative of PERCEPIO has been
 * advised of the possibility of such damages, or for any claim by any third
 * party. Some jurisdictions do not allow the exclusion or limitation of
 * incidental, consequential or special damages, or the exclusion of implied
 * warranties or limitations on how long an implied warranty may last, so the
 * above limitations may not apply to you.
 *
 * Tabs are used for indent in this file (1 tab = 4 spaces)
 *
 * Copyright Percepio AB, 2017.
 * www.percepio.com
 ******************************************************************************/

#include "trcRecorder.h"

#if (TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_STREAMING)  
#if (TRC_USE_TRACEALYZER_RECORDER == 1)

#include <stdio.h>
#include "ff.h"
static FIL trc_file;
static char trc_file_open = 0;
static SemaphoreHandle_t file_mutex;

// openFile() is called during init() before our SPI and File FS operations are ready, so we do not open the file here
void openFile(char* fileName)
{
    if (NULL == file_mutex)
    {
        file_mutex = xSemaphoreCreateMutex();
    }
}

int32_t writeToFile(void* data, uint32_t size, int32_t *ptrBytesWritten)
{
    // Sanity checks
    if (NULL == data || size == 0) {
        puts("ERROR EXCEPTION");
        return -1;
    }

    // Open the file if it has not been opened already
    if (!trc_file_open)
    {
        // Always create a new file
        if (FR_OK != f_open(&trc_file, "1:trc.psf", (FA_WRITE | FA_CREATE_ALWAYS))) {
            puts("ERROR: Could not open FreeRTOS trace file.");
        }
        else {
            puts("FreeRTOS trace file has been opened.");
            trc_file_open = 1;
        }
    }

    // Write the data to the file:
    UINT bw = 0;
    FRESULT res = FR_OK;
    xSemaphoreTake(file_mutex, portMAX_DELAY);
    {
        if (!trc_file_open) {
            puts("ERROR: FreeRTOS trace file was not open.");
        }
        else if (FR_OK == (res = f_write(&trc_file, data, size, &bw)))
        {
            // Sync the file periodically:
            f_sync(&trc_file);

            *ptrBytesWritten = (int32_t) bw;
        }
        else {
            *ptrBytesWritten = 0;
        }
    }
    xSemaphoreGive(file_mutex);

    // Return 0 upon success:
    return (size == bw) ? 0 : -1;
}

void closeFile(void)
{
    xSemaphoreTake(file_mutex, portMAX_DELAY);
    if (trc_file_open) {
        f_close(&trc_file);
        trc_file_open = 0;
    }
    xSemaphoreGive(file_mutex);
}

#endif /*(TRC_USE_TRACEALYZER_RECORDER == 1)*/
#endif /*(TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_STREAMING)*/
