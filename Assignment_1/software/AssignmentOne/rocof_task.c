#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"
#include "shared_state.h"

/* Created in main.c */
extern QueueHandle_t rocofQueue;

void ROCOFTask(void *pvParameters)
{
    float currentFrequencyHz;
    float lastFrequencyHz = 50.0f;
    float rocofValue;

    while (1)
    {
        /* Wait for a new frequency from FrequencyTask */
        if (xQueueReceive(rocofQueue, &currentFrequencyHz, portMAX_DELAY) == pdTRUE)
        {
            if ((currentFrequencyHz + lastFrequencyHz) != 0.0f)
            {
                rocofValue =
                    (currentFrequencyHz - lastFrequencyHz) * 2.0f * currentFrequencyHz * lastFrequencyHz / (currentFrequencyHz + lastFrequencyHz);
            }
            else
            {
                rocofValue = 0.0f;
            }

            if (xSemaphoreTake(sharedStateMutex, portMAX_DELAY) == pdTRUE)
            {
                currentROCOF = rocofValue;
                xSemaphoreGive(sharedStateMutex);
            }

            lastFrequencyHz = currentFrequencyHz;
        }
    }
}
// Explanation of the above (with good formatting, i.e. multi line comments, indentation, etc.):
/*
This code defines a FreeRTOS task called ROCOFTask. The task continuously waits for
frequency values to be sent to the rocofQueue by the FrequencyTask. When it receives a frequency value, it calculates the ROCOF (Rate of Change of Frequency) using the formula: ROCOF = (f_current - f_last) * 2 * f_current * f_last / (f_current + f_last). This formula is derived from the definition of ROCOF and provides a more accurate calculation that accounts for both the current and last frequency values. The calculated ROCOF value is then stored in the shared variable currentROCOF, which is protected by a mutex (sharedStateMutex) to ensure thread safety. Finally, it updates lastFrequencyHz with the current frequency for use in the next calculation. This task is responsible for monitoring the rate of change of frequency, which is crucial for making decisions about load shedding in response to system instability.
*/

