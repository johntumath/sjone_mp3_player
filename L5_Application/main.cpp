#include "tasks.hpp"

int main(void)
{
    scheduler_add_task(new terminalTask(PRIORITY_HIGH));

    scheduler_start();
    return -1;
}
