#include "Main.h"

/* Queue / mutex globals declared in Main.h */
QueueHandle_t qFrequencySamples = NULL;
QueueHandle_t qFrequencyToROCOF = NULL;
QueueHandle_t qFrequencyToDecision = NULL;
QueueHandle_t qROCOFToDecision = NULL;
QueueHandle_t qButtonToDecision = NULL;
QueueHandle_t qDecisionToLoadControl = NULL;
QueueHandle_t qResponseMeasurements = NULL;

SemaphoreHandle_t gStateMutex = NULL;
SharedSystemState gSystemState;

float absf_local(float x)
{
    return (x < 0.0f) ? -x : x;
}

/* ----- Configuration for this test stage ----- */
#define SWITCH_POLL_MS  10

/* ----- Local queue for switch state ----- */
static QueueHandle_t gSwitchQueue = NULL;

/*
 * Stub: loads forced OFF by relay logic.
 * Later this will be replaced by proper relay state / load control task.
 *
 * Bit = 1 -> relay is forcing that load OFF
 * Bit = 0 -> relay allows that load
 */
static volatile uint32_t gRelayForcedOffMask = 0;

/* ----- Local prototypes ----- */
static void SwitchPollTask(void *pvParameters);
static void RedLedTask(void *pvParameters);
static void GreenBlinkTask(void *pvParameters);

static uint32_t getRelayForcedOffMask(void);
static void setRelayForcedOffMask(uint32_t newMask);

int main(void)
{
    /* Simple queue for current switch snapshot */
    gSwitchQueue = xQueueCreate(1, sizeof(uint32_t));
    if (gSwitchQueue == NULL)
    {
        while (1) { }
    }

    xTaskCreate(
        SwitchPollTask,
        "SwitchPoll",
        TASK_STACKSIZE,
        NULL,
        3,
        NULL
    );

    xTaskCreate(
        RedLedTask,
        "RedLed",
        TASK_STACKSIZE,
        NULL,
        2,
        NULL
    );

    xTaskCreate(
        GreenBlinkTask,
        "GreenBlink",
        TASK_STACKSIZE,
        NULL,
        1,
        NULL
    );

    vTaskStartScheduler();

    while (1) { }

    return 0;
}

/*
 * Poll slide switches because they do not have IRQs.
 * Sends latest switch state to the LED/output task.
 */
static void SwitchPollTask(void *pvParameters)
{
    uint32_t switchValue;

    (void)pvParameters;

    while (1)
    {
        switchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);

        /* Keep only the load bits we care about */
        switchValue &= ((1u << LOAD_COUNT) - 1u);

        /*
         * Queue length is 1, so overwrite keeps the latest state only.
         * This is ideal for "current switch snapshot" style data.
         */
        xQueueOverwrite(gSwitchQueue, &switchValue);

        vTaskDelay(pdMS_TO_TICKS(SWITCH_POLL_MS));
    }
}

/*
 * Red LEDs show loads that are requested ON by the switches
 * and not forced OFF by relay logic.
 */
static void RedLedTask(void *pvParameters)
{
    uint32_t requestedLoads = 0;
    uint32_t forcedOffMask;
    uint32_t redLedOutput;

    (void)pvParameters;

    while (1)
    {
        xQueueReceive(gSwitchQueue, &requestedLoads, portMAX_DELAY);

        forcedOffMask = getRelayForcedOffMask();

        /*
         * Load is ON only if:
         * - switch requests it ON
         * - relay is not forcing it OFF
         */
        redLedOutput = requestedLoads & (~forcedOffMask);
        redLedOutput &= ((1u << LOAD_COUNT) - 1u);

        IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, redLedOutput);
    }
}

/*
 * Simple heartbeat so you can see FreeRTOS is alive.
 * Later green LEDs will represent relay-shed loads instead.
 */
static void GreenBlinkTask(void *pvParameters)
{
    uint32_t ledPattern = 0;

    (void)pvParameters;

    while (1)
    {
        ledPattern ^= ((1u << LOAD_COUNT) - 1u);
        IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, ledPattern);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ----- Stub relay override helpers ----- */
static uint32_t getRelayForcedOffMask(void)
{
    return gRelayForcedOffMask;
}

static void setRelayForcedOffMask(uint32_t newMask)
{
    gRelayForcedOffMask = newMask & ((1u << LOAD_COUNT) - 1u);
}
