#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/semphr.h"
#include "shared_state.h"

#include "system.h"
#include "altera_avalon_pio_regs.h"

static void record_first_shed_delay(TickType_t nowTick)
{
    unsigned int dt_ms =
        (unsigned int)((nowTick - detectionTick) * portTICK_PERIOD_MS);

    firstShedTick = nowTick;

    printf("FIRST SHED OUTPUT DELAY = %u ms\n", dt_ms);

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
    unsigned int prevActual = 0xFFFFFFFFU;

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

            if (!maintenance && timingArmed && !firstShedDone)
            {
                if ((prevActual != 0xFFFFFFFFU) &&
                    (actual != prevActual) &&
                    (shed != 0U))
                {
                    record_first_shed_delay(xTaskGetTickCount());
                }
            }

            prevActual = actual;

            xSemaphoreGive(sharedStateMutex);
        }

        IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, redLeds);
        IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, greenLeds);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
