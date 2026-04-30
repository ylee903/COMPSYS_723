#include <stdio.h>

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/semphr.h"
#include "shared_state.h"

#include "system.h"
#include "altera_avalon_pio_regs.h"

static void record_first_shed_actuation_delay(TickType_t nowTick)
{
    unsigned int dt_ms;

    dt_ms = (unsigned int)((nowTick - detectionTick) * portTICK_PERIOD_MS);
    firstShedTick = nowTick;

    printf("ISR TO FIRST SHED ACTUATION DELAY = %u ms\n", dt_ms);

    recentTimes[recentIndex] = dt_ms;
    recentIndex = (recentIndex + 1U) % RECENT_TIMES_COUNT;

    if (recentCount < RECENT_TIMES_COUNT)
    {
        recentCount++;
    }

    totalTime += dt_ms;
    measurementCount++;

    if (dt_ms < minTime)
    {
        minTime = dt_ms;
    }

    if (dt_ms > maxTime)
    {
        maxTime = dt_ms;
    }

    firstShedDone = 1;
}

void LoadControlTask(void *pvParameters)
{
    unsigned int requested;
    unsigned int shed;
    unsigned int maintenance;
    unsigned int actual;
    unsigned int redLeds;
    unsigned int greenLeds;
    TickType_t nowTick;

    while (1)
    {
        if (xSemaphoreTake(sharedStateMutex, portMAX_DELAY) == pdTRUE)
        {
            requested = allowedLoads & 0x1F;
            shed = relayShedLoads & 0x1F;
            maintenance = maintenanceMode;

            if (maintenance)
            {
                actual = requested;
                greenLeds = 0;
            }
            else
            {
                actual = requested & (~shed);
                actual &= 0x1F;
                greenLeds = shed & 0x1F;
            }

            actualLoads = actual;
            redLeds = actual & 0x1F;

            if (!maintenance && timingArmed && !firstShedDone && (shed != 0))
            {
                nowTick = xTaskGetTickCount();
                record_first_shed_actuation_delay(nowTick);
            }

            xSemaphoreGive(sharedStateMutex);
        }

        IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, redLeds);
        IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, greenLeds);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Explanation of the above (with good formatting, i.e. multi line comments, indentation, etc.):
/*
This code defines a FreeRTOS task called LoadControlTask. The task continuously runs in an
infinite loop, where it takes the sharedStateMutex to safely access shared variables that represent the requested loads, shed loads, maintenance mode, and actual loads. Based on whether the system is in maintenance mode or not, it determines which loads should be active (actual) and which LEDs should be turned on (red for active loads, green for shed loads).
If the system is not in maintenance mode and timing is armed but the first shed actuation has not been done yet,
 it records the delay from detection to actuation. Finally, it updates the LED states and delays for 50 ms before repeating the process.
  This task is responsible for controlling the load shedding logic and providing visual feedback through the LEDs.
*/
