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
