#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"
#include "shared_state.h"

/* Created in main.c */
extern QueueHandle_t thresholdQueue;

void KeyboardTask(void *pvParameters)
{
    unsigned char key;

    while (1)
    {
        /* Wait for key from ISR */
        if (xQueueReceive(thresholdQueue, &key, portMAX_DELAY) == pdTRUE)
        {
            if (xSemaphoreTake(sharedStateMutex, portMAX_DELAY) == pdTRUE)
            {
                switch (key)
                {
                    case 0x2d:   /* R */
                        rocofThreshold += 2.5f;
                        break;

                    case 0x24:   /* E */
                        rocofThreshold -= 2.5f;
                        if (rocofThreshold < 0.0f)
                            rocofThreshold = 0.0f;
                        break;

                    case 0x2b:   /* F */
                        frequencyThreshold += 0.1f;
                        break;

                    case 0x23:   /* D */
                        frequencyThreshold -= 0.1f;
                        break;

                    default:
                        break;
                }

                xSemaphoreGive(sharedStateMutex);
            }
        }
    }
}
