#include "tasks.hpp"
#include "LCDdisplay.h"
#include "FreeRTOS.h"

LCD_display display(0xe4);

void init_display(void*)
{
    char name[] = "Quick and Dirty MP3";
    int len =19;
    vTaskDelay(1);
    display.init();
    for(int i=0; i<len; ++i){
        display.write_char(name[i]);
        vTaskDelay(10);
    }
}


int main(void)
{
    xTaskCreate(init_display,"init display",1024, nullptr, 2, nullptr);

    
    scheduler_add_task(new terminalTask(PRIORITY_HIGH));

    scheduler_start();
    return -1;
}
