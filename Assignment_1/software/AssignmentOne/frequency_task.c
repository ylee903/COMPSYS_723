#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"
#include "shared_state.h"

/* Queue created in main.c */
extern QueueHandle_t freqQueue;
extern QueueHandle_t rocofQueue;

void FrequencyTask(void *pvParameters)
{
    unsigned int sampleCount;
    float frequencyHz;

    while (1)
    {
        if (xQueueReceive(freqQueue, &sampleCount, portMAX_DELAY) == pdTRUE)
        {
            if (sampleCount != 0)
            {
                frequencyHz = 16000.0f / (float)sampleCount;

                if (xSemaphoreTake(sharedStateMutex, portMAX_DELAY) == pdTRUE)
                {
                    currentFrequencyHz = frequencyHz;
                    xSemaphoreGive(sharedStateMutex);
                }

                xQueueSend(rocofQueue, &frequencyHz, 0);
            }
        }
    }
}

// Explanation of the above (with good formatting, i.e. multi line comments, indentation, etc.):
/*
This code defines a FreeRTOS task called FrequencyTask. The task continuously waits for frequency sample                                                                                                    
counts to be sent to the freqQueue. When it receives a sample count, it calculates the frequency in Hz using the formula frequency = 16000 / sampleCount (assuming a sampling rate of 16 kHz). It then updates the shared variable currentFrequencyHz with the calculated frequency, ensuring that access to this variable is protected by a mutex (sharedStateMutex). Finally, it sends the calculated frequency to another queue (rocofQueue) for further processing by other tasks, such as calculating the ROCOF (Rate of Change of Frequency).
*/  