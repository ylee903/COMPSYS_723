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

// Explanation of the above (with good formatting, i.e. multi line comments, indentation, etc.):
/*
This code defines a FreeRTOS task called KeyboardTask. The task continuously waits for key codes    
to be sent to the thresholdQueue by the KeyboardISR. When it receives a key code, it takes the sharedStateMutex to safely access and modify the shared variables frequencyThreshold and rocofThreshold based on the key pressed. The keys 'R' and 'E' are used to increase and decrease the ROCOF threshold, while 'F' and 'D' are used to increase and decrease the frequency threshold. After updating the thresholds, it gives back the mutex. This allows the user to adjust the thresholds for load shedding in real-time using the keyboard.
*/