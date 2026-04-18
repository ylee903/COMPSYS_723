#include <stdio.h>
#include <math.h>

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/semphr.h"
#include "shared_state.h"

#define LOAD_MASK 0x1F

extern SemaphoreHandle_t maintenanceSemaphore;

static void reset_timing_flags(void)
{
    timingArmed = 0;
    firstShedDone = 0;
}

static int find_lowest_priority_on_not_shed(unsigned int allowed, unsigned int shed)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        if ((allowed & (1U << i)) && !(shed & (1U << i)))
        {
            return i;
        }
    }
    return -1;
}

static int find_highest_priority_shed(unsigned int shed)
{
    int i;
    for (i = 4; i >= 0; i--)
    {
        if (shed & (1U << i))
        {
            return i;
        }
    }
    return -1;
}

void DecisionTask(void *pvParameters)
{
    unsigned int requested;
    unsigned int allowed;
    unsigned int shed;
    unsigned int maintenance;
    unsigned int unstable;
    unsigned int prevUnstable = 0;

    TickType_t observationStartTick = 0;
    TickType_t now;

    while (1)
    {
        if (xSemaphoreTake(maintenanceSemaphore, 0) == pdTRUE)
        {
            if (xSemaphoreTake(sharedStateMutex, portMAX_DELAY) == pdTRUE)
            {
                maintenanceMode = !maintenanceMode;
                printf("BUTTON PRESS -> maintenanceMode = %u\n", maintenanceMode);
                xSemaphoreGive(sharedStateMutex);
            }
        }

        if (xSemaphoreTake(sharedStateMutex, portMAX_DELAY) == pdTRUE)
        {
            requested   = userRequestedLoads & LOAD_MASK;
            allowed     = allowedLoads & LOAD_MASK;
            shed        = relayShedLoads & LOAD_MASK;
            maintenance = maintenanceMode;

            /* Get current tick BEFORE using it for timing */
            now = xTaskGetTickCount();

            if (maintenance)
            {
                managingLoads = 0;
                systemUnstable = 0;
                relayShedLoads = 0;
                allowedLoads = requested;
                prevUnstable = 0;

                reset_timing_flags();

                printf("MAINT | Freq=%.2f Hz | ROCOF=%.2f | Fth=%.2f | Rth=%.2f\n",
                       currentFrequencyHz, currentROCOF,
                       frequencyThreshold, rocofThreshold);

                xSemaphoreGive(sharedStateMutex);
                vTaskDelay(pdMS_TO_TICKS(20));
                continue;
            }

            if ((currentFrequencyHz < frequencyThreshold) ||
                (fabsf(currentROCOF) > rocofThreshold))
            {
                unstable = 1;
            }
            else
            {
                unstable = 0;
            }

            /* Arm timing when instability first occurs */
            if (unstable && !timingArmed)
            {
                detectionTick = xTaskGetTickCount();
                timingArmed = 1;
                firstShedDone = 0;
            }

            systemUnstable = unstable;

            printf("Freq=%.2f Hz | ROCOF=%.2f | Fth=%.2f | Rth=%.2f | unstable=%u | req=%u | allow=%u | shed=%u\n",
                   currentFrequencyHz, currentROCOF,
                   frequencyThreshold, rocofThreshold,
                   unstable, requested, allowed, shed);

            /* Users may always turn OFF loads manually */
            allowed &= requested;
            shed &= requested;

            if (!managingLoads)
            {
                /* In normal mode, loads follow switches directly */
                allowed = requested;

                if (unstable)
                {
                    int loadToShed = find_lowest_priority_on_not_shed(allowed, shed);
                    if (loadToShed >= 0)
                    {
                        shed |= (1U << loadToShed);
                        managingLoads = 1;
                        observationStartTick = now;
                    }
                }
            }
            else
            {
                if (unstable != prevUnstable)
                {
                    /* Reset the 500 ms observation period when state changes */
                    observationStartTick = now;
                }
                else
                {
                    if ((now - observationStartTick) >= pdMS_TO_TICKS(500))
                    {
                        if (unstable)
                        {
                            int loadToShed = find_lowest_priority_on_not_shed(allowed, shed);
                            if (loadToShed >= 0)
                            {
                                shed |= (1U << loadToShed);
                            }
                        }
                        else
                        {
                            int loadToRestore = find_highest_priority_shed(shed);
                            if (loadToRestore >= 0)
                            {
                                shed &= ~(1U << loadToRestore);
                            }

                            if ((shed & LOAD_MASK) == 0)
                            {
                                managingLoads = 0;
                                allowed = requested;
                                reset_timing_flags();
                            }
                        }

                        observationStartTick = now;
                    }
                }
            }

            allowedLoads = allowed & LOAD_MASK;
            relayShedLoads = shed & LOAD_MASK;
            prevUnstable = unstable;

            xSemaphoreGive(sharedStateMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
