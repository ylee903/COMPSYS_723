#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/semphr.h"
#include "shared_state.h"

#include "system.h"
#include "altera_avalon_pio_regs.h"

void SwitchPollTask(void *pvParameters)
{
    unsigned int switchValue;

    while (1)
    {
        switchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
        // What does the above line do? It reads the current state of the slide switches from the hardware register. The value is a bitmask where each bit represents the state of a switch (1 for on, 0 for off). Since we only have 5 switches, we mask it with 0x1F to ignore any higher bits that might be present.
        switchValue &= 0x1F;

        if (xSemaphoreTake(sharedStateMutex, portMAX_DELAY) == pdTRUE)
        {
            userRequestedLoads = switchValue;
            xSemaphoreGive(sharedStateMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
// Explanation of the above (with good formatting, i.e. multi line comments, indentation, etc.):
/*
This code defines a FreeRTOS task called SwitchPollTask. The task continuously runs in an
infinite loop, where it reads the state of the slide switches from the hardware register. The value read is masked to only consider the lower 5 bits, which correspond to the 5 switches. The task then takes the sharedStateMutex to safely update the shared variable userRequestedLoads with the current switch states. After updating, it gives back the mutex and delays for 50 ms before repeating the process. This task is responsible for monitoring the user input from the slide switches and updating the system's requested loads accordingly.
*/
