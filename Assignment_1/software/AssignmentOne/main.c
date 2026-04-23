#include <stdio.h>

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"
#include "FreeRTOS/timers.h"
#include "shared_state.h"
#include "system.h"
#include "sys/alt_irq.h"
#include "altera_avalon_pio_regs.h"
#include "altera_up_avalon_ps2.h"
#include "isr_handlers.h"

/* Task prototypes */
void FrequencyTask(void *pvParameters);
void ROCOFTask(void *pvParameters);
void DecisionTask(void *pvParameters);
void LoadControlTask(void *pvParameters);
void SwitchPollTask(void *pvParameters);
void KeyboardTask(void *pvParameters);
void VGATask(void *pvParameters);

/* Task priorities */
#define PRIORITY_DECISION      (tskIDLE_PRIORITY + 5)
#define PRIORITY_FREQ          (tskIDLE_PRIORITY + 4)
#define PRIORITY_ROCOF         (tskIDLE_PRIORITY + 4)
#define PRIORITY_LOAD          (tskIDLE_PRIORITY + 3)
#define PRIORITY_SWITCH        (tskIDLE_PRIORITY + 2)
#define PRIORITY_KEYBOARD      (tskIDLE_PRIORITY + 2)
#define PRIORITY_VGA           (tskIDLE_PRIORITY + 1)

/* Global queues */
QueueHandle_t freqQueue = NULL;
QueueHandle_t rocofQueue = NULL;
QueueHandle_t switchQueue = NULL;
QueueHandle_t thresholdQueue = NULL;
QueueHandle_t loadCommandQueue = NULL;

/* Global synchronisation objects */
SemaphoreHandle_t maintenanceSemaphore = NULL;
SemaphoreHandle_t analyserSemaphore = NULL;

/* Global timer */
TimerHandle_t managementTimer = NULL;

int main(void)
{
    printf("LCFR starting...\n");

    /* Create queues */
    freqQueue = xQueueCreate(8, sizeof(unsigned int));
    rocofQueue = xQueueCreate(8, sizeof(float));
    switchQueue = xQueueCreate(8, sizeof(unsigned int));
    thresholdQueue = xQueueCreate(8, sizeof(unsigned char));
    loadCommandQueue = xQueueCreate(8, sizeof(unsigned int));

    /* Create semaphores and mutex */
    maintenanceSemaphore = xSemaphoreCreateBinary();
    analyserSemaphore = xSemaphoreCreateBinary();
    sharedStateMutex = xSemaphoreCreateMutex();

    /* Create tasks */
    xTaskCreate(DecisionTask,    "Decision", 2048, NULL, PRIORITY_DECISION, NULL);
    xTaskCreate(FrequencyTask,   "Freq",     1024, NULL, PRIORITY_FREQ, NULL);
    xTaskCreate(ROCOFTask,       "ROCOF",    1024, NULL, PRIORITY_ROCOF, NULL);
    xTaskCreate(LoadControlTask, "Load",     1024, NULL, PRIORITY_LOAD, NULL);
    xTaskCreate(SwitchPollTask,  "Switch",   1024, NULL, PRIORITY_SWITCH, NULL);
    xTaskCreate(KeyboardTask,    "Keyboard", 1024, NULL, PRIORITY_KEYBOARD, NULL);
    xTaskCreate(VGATask,         "VGA",      2048, NULL, PRIORITY_VGA, NULL);

    alt_up_ps2_dev *ps2_device = alt_up_ps2_open_dev(PS2_NAME);
    if (ps2_device != NULL)
    {
        alt_up_ps2_enable_read_interrupt(ps2_device);
        alt_irq_register(PS2_IRQ, ps2_device, KeyboardISR);
    }

    /* Clear any stale push-button edge first, then enable ISR */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0x7);
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PUSH_BUTTON_BASE, 0x4);

    /* Register push-button ISR */
    alt_irq_register(PUSH_BUTTON_IRQ, NULL, PushButtonISR);

    /* Register analyser ISR */
    alt_irq_register(FREQUENCY_ANALYSER_IRQ, NULL, FrequencyAnalyserISR);

    systemStartTick = xTaskGetTickCount();
    /* Start scheduler */
    vTaskStartScheduler();

    /* Should never get here */
    while (1)
    {
    }

    return 0;
}
// Explanation of the above (with good formatting, i.e. multi line comments, indentation, etc.):
/*
 * This is the main entry point for the Load Control and Frequency Response (LCFR) system.
 * It initializes the necessary queues, semaphores, and tasks, and then starts the FreeRTOS scheduler.
 *
 * The system consists of several tasks:
 * - DecisionTask: Makes decisions on load shedding based on frequency and ROCOF.
 * - FrequencyTask: Measures the frequency of the power system.
 * - ROCOFTask: Calculates the Rate of Change of Frequency (ROCOF).
 * - LoadControlTask: Controls the load shedding relays.
 * - SwitchPollTask: Polls the user switches to determine requested loads.
 * - KeyboardTask: Handles keyboard input for maintenance mode.
 * - VGATask: Updates the VGA display with system status.
 *
 * The system also sets up interrupt service routines (ISRs) for handling push-button presses and PS/2 keyboard input.
 */
