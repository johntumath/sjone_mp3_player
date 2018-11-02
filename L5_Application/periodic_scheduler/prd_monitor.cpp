/*
 *     SocialLedge.com - Copyright (C) 2013
 *
 *     This file is part of free software framework for embedded processors.
 *     You can use it and/or distribute it as long as this copyright header
 *     remains unmodified.  The code is free for personal use and requires
 *     permission to use in a commercial product.
 *
 *      THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 *      OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 *      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 *      I SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 *      CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *     You can reach the author of this software at :
 *          p r e e t . w i k i @ g m a i l . c o m
 */

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "tasks.hpp"
#include "storage.hpp"
#include "lpc_sys.h"
#include "periodic_callback.h"



/// Enumeration of the periodic task frequency
typedef enum {
    prd_1Hz = 0,
    prd_10Hz,
    prd_100Hz,
    prd_1000Hz,
    prd_total
} prd_task_t;

/// Enum to delay time in milliseconds
static const uint32_t prd_Hz_to_ms_table[] = { 1000, 100, 10, 1 } ;

/// Struct of a periodic task
typedef struct {
    SemaphoreHandle_t run_complete_sem;         ///< Semaphore that is given after periodic task run is complete
    uint32_t prev_sleep_time_ms;                ///< Used for vTaskDelayUntil()

    uint32_t count;                             ///< Count given to the periodic task callback
    void (*period_callback) (uint32_t count);   ///< Actual callback function
} periodic_task_t;

/// Semaphores that the period tasks wait upon
static periodic_task_t periodic_task[prd_total];

/// Periodic Task template
void period_task(prd_task_t type)
{
    periodic_task_t *task = &(periodic_task[type]);

    // Sleep this periodic task for its intended period
    vTaskDelayUntil(&(task->prev_sleep_time_ms), prd_Hz_to_ms_table[type]);

    // Call the periodic callback and increment its acount
    task->period_callback(task->count);
    ++(task->count);

    // Give the signal that the tasks' run is complete
    xSemaphoreGive(task->run_complete_sem);
}

/// @{ These are the actual FreeRTOS tasks that call the period functions
void period_task_1Hz(void *p)    { while(1) { period_task(prd_1Hz);    } }
void period_task_10Hz(void *p)   { while(1) { period_task(prd_10Hz);   } }
void period_task_100Hz(void *p)  { while(1) { period_task(prd_100Hz);  } }
void period_task_1000Hz(void *p) { while(1) { period_task(prd_1000Hz); } }
/// @}

periodicSchedulerTask::periodicSchedulerTask(bool kHz_enabled) :
    scheduler_task("prdmon", PERIOD_MONITOR_TASK_STACK_SIZE_BYTES, PRIORITY_CRITICAL + PRIORITY_CRITICAL + 5),
    mKHz(kHz_enabled)
{
    periodic_task[prd_1Hz].period_callback  = period_1Hz;
    periodic_task[prd_10Hz].period_callback  = period_10Hz;
    periodic_task[prd_100Hz].period_callback  = period_100Hz;
    periodic_task[prd_1000Hz].period_callback  = period_1000Hz;

    // Create the semaphores first before creating the actual periodic tasks
    periodic_task[prd_1Hz].run_complete_sem   = xSemaphoreCreateBinary();
    periodic_task[prd_10Hz].run_complete_sem  = xSemaphoreCreateBinary();
    periodic_task[prd_100Hz].run_complete_sem = xSemaphoreCreateBinary();

    // Optional: Provide names of the FreeRTOS objects for the Trace Facility
    vTraceSetSemaphoreName(periodic_task[prd_1Hz].run_complete_sem, "1Hz_Sem");
    vTraceSetSemaphoreName(periodic_task[prd_10Hz].run_complete_sem, "10Hz_Sem");
    vTraceSetSemaphoreName(periodic_task[prd_100Hz].run_complete_sem, "100Hz_Sem");

    // Create the FreeRTOS tasks, these will only run once we start giving their semaphores
    xTaskCreate(period_task_1Hz, "1Hz", PERIOD_TASKS_STACK_SIZE_BYTES/4, NULL, PRIORITY_CRITICAL + 1, NULL);
    xTaskCreate(period_task_10Hz, "10Hz", PERIOD_TASKS_STACK_SIZE_BYTES/4, NULL, PRIORITY_CRITICAL + 2, NULL);
    xTaskCreate(period_task_100Hz, "100Hz", PERIOD_TASKS_STACK_SIZE_BYTES/4, NULL, PRIORITY_CRITICAL + 3, NULL);

    // If 1Khz is enabled:
    if (mKHz) {
        periodic_task[prd_1000Hz].run_complete_sem = xSemaphoreCreateBinary();
        vTraceSetSemaphoreName(periodic_task[prd_1000Hz].run_complete_sem, "1000Hz_Sem");
        xTaskCreate(period_task_1000Hz, "1000Hz", PERIOD_TASKS_STACK_SIZE_BYTES/4, NULL, PRIORITY_CRITICAL + 4, NULL);
    }
}

bool periodicSchedulerTask::init(void)
{
    return period_init();
}

bool periodicSchedulerTask::regTlm(void)
{
    return period_reg_tlm();
}


bool periodicSchedulerTask::run(void *p)
{
    for(;;)
    {
        // Periodic Monitor runs at 1Khz
        vTaskDelay(OS_MS(1));

        // 1Khz task is run every time unless it was disabled
        if (mKHz) {
            handlePeriodicSemaphore(prd_1000Hz, 1);
        }

        // 100Hz and below run every 10th time:
        if (handlePeriodicSemaphore(prd_100Hz, 10)) {
            if (handlePeriodicSemaphore(prd_10Hz, 10)) {
                if (handlePeriodicSemaphore(prd_1Hz, 10)) {
                    ; // 1Hz task ran; nothing to do
                }
            }
        }
    }

    return true;
}

bool periodicSchedulerTask::handlePeriodicSemaphore(const uint8_t index, const uint8_t frequency)
{
    bool task_should_have_run = false;
    static uint8_t counters[prd_total] = { 0 };
    static const char * overrunMsg[] = { "1Hz task overrun\n", "10Hz task overrun\n", "100Hz task overrun\n", "1000Hz task overrun\n" };

    if (frequency == ++counters[index])
    {
        counters[index] = 0;
        task_should_have_run = true;

        if (!xSemaphoreTake(periodic_task[index].run_complete_sem, 0))
        {
            // Write a message to a file for indication
            puts(overrunMsg[index]);
            Storage::append("restart.txt", overrunMsg[index], strlen(overrunMsg[index]), 0);

            // Reboot
            sys_reboot_abnormal();
        }
    }

    return task_should_have_run;
}
