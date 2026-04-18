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
