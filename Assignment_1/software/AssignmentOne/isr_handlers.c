#include "isr_handlers.h"
#include "shared_state.h"

#include "system.h"
#include "io.h"
#include "sys/alt_irq.h"
#include "altera_avalon_pio_regs.h"
#include "altera_up_avalon_ps2.h"

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"
#include "FreeRTOS/task.h"

/* Created in main.c */
extern QueueHandle_t freqQueue;          /* unsigned int sample count */
extern QueueHandle_t thresholdQueue;     /* unsigned char key code */

extern SemaphoreHandle_t maintenanceSemaphore;

void FrequencyAnalyserISR(void *context)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    unsigned int sampleCount;

    lastAnalyserIsrTick = xTaskGetTickCountFromISR();

    sampleCount = IORD(FREQUENCY_ANALYSER_BASE, 0);

    xQueueSendFromISR(freqQueue, &sampleCount, &xHigherPriorityTaskWoken);

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void PushButtonISR(void *context)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    unsigned int edgeCapture;

    /* Read which button was pressed */
    edgeCapture = IORD_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE);

    /* Clear interrupt */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, edgeCapture);

    /* Only respond to KEY3 */
    if (edgeCapture & 0x4)
    {
        xSemaphoreGiveFromISR(maintenanceSemaphore, &xHigherPriorityTaskWoken);
    }

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void KeyboardISR(void *context, alt_u32 id)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    unsigned char key = 0;

    /* Decode one scancode byte */
    decode_scancode(context, NULL, &key, NULL);

    /* Send key to KeyboardTask */
    if (key != 0)
    {
        xQueueSendFromISR(thresholdQueue, &key, &xHigherPriorityTaskWoken);
    }

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

// Explanation of the above (with good formatting, i.e. multi line comments, indentation, etc.):
/*
This code defines three interrupt service routines (ISRs) for handling events from the frequency analyser, push buttons, and keyboard.      
1. FrequencyAnalyserISR: This ISR is triggered by the frequency analyser hardware. It reads the sample count from the hardware register and sends it to the freqQueue for processing by the FrequencyTask. It also records the tick count at which the ISR was called.      
2. PushButtonISR: This ISR is triggered by the push buttons. It reads which button was pressed, clears the interrupt, and if KEY3 was pressed, it gives the maintenanceSemaphore to signal the DecisionTask to toggle maintenance mode.      
3. KeyboardISR: This ISR is triggered by keyboard events. It decodes the scancode to get the key code and sends it to the thresholdQueue for processing by the KeyboardTask, which will adjust the frequency and ROCOF thresholds based on user input. All ISRs use the FreeRTOS API to communicate with tasks and ensure that if a higher priority task was woken by the ISR, it will yield to that task immediately after the ISR finishes.
*/          