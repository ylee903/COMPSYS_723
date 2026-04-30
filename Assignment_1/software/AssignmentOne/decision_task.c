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
    timingArmed = 0;   // Timing is armed when instability is detected but no loads have been shed yet. This flag is used to measure the delay from detection to actuation for the first load shed.
    firstShedDone = 0; // This flag indicates whether the first load shedding actuation has been done since instability was detected. It is used to ensure that the delay from detection to actuation is only recorded once per instability event.
}

static int find_lowest_priority_on_not_shed(unsigned int allowed, unsigned int shed)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        if ((allowed & (1U << i)) && !(shed & (1U << i))) // Find the lowest priority load that is currently allowed to be on and not already shed
        {
            return i;
        }
    }
    return -1;
}

static int find_highest_priority_shed(unsigned int shed) // Find the highest priority load that is currently shed
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
                detectionTick = lastAnalyserIsrTick;
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
            // What are the above variables for? allowedLoads represents the loads that are currently allowed to be on based on user requests and system decisions. relayShedLoads represents the loads that are currently shed (turned off) by the relay. Both of these are updated based on the decision logic in this task.
            prevUnstable = unstable;

            xSemaphoreGive(sharedStateMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// Explanation of the above (with good formatting, i.e. multi line comments, indentation, etc.):
/*
This code defines a FreeRTOS task called DecisionTask, which is responsible for managing the load shedding logic based on the system's frequency and ROCOF (Rate of Change of Frequency) measurements. The task continuously checks for user requests to turn loads on or off, monitors the system's stability, and decides which loads to shed if the system becomes unstable.
Key components of the code include:
1. **Shared State Management**: The task uses a mutex (sharedStateMutex) to
    safely access and modify shared state variables such as userRequestedLoads, allowedLoads, relayShedLoads, maintenanceMode, currentFrequencyHz, currentROCOF, frequencyThreshold, rocofThreshold, managingLoads, and systemUnstable.
2. **Maintenance Mode Handling**: If the maintenance mode is toggled, the task resets all load management and instability flags, allowing all loads to be turned on without restrictions.
3. **Instability Detection**: The task checks if the current frequency is below a defined
    threshold or if the absolute value of ROCOF exceeds a defined threshold to determine if the system is unstable.
4. **Load Shedding Logic**: If the system is unstable, the task identifies the
    lowest priority load that is currently allowed but not yet shed and sheds it. If the system becomes stable again, it identifies the highest priority load that is currently shed and restores it.   
5. **Timing Logic**: The task uses a timing mechanism to ensure that loads are not shed or restored too rapidly, implementing a 500 ms observation period before making further decisions after a change in stability.
Overall, this task is crucial for maintaining the stability of the system by dynamically managing the loads based on real-time measurements and user inputs, ensuring that the system can respond effectively to changes in frequency and ROCOF while also allowing for manual control through maintenance mode.
*/                                                                      

