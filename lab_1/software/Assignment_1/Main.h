#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include <system.h>
#include <io.h>
#include <sys/alt_irq.h>
#include <altera_avalon_pio_regs.h>

#ifndef portYIELD_FROM_ISR
#define portYIELD_FROM_ISR(x) \
    do { if ((x) != pdFALSE) taskYIELD(); } while (0)
#endif

#define LOAD_COUNT 5
#define TASK_STACKSIZE 2048

#define FREQUENCY_TASK_PRIORITY         15
#define ROCOF_TASK_PRIORITY             14
#define KEYBOARD_TASK_PRIORITY          13
#define DECISION_TASK_PRIORITY          12
#define LOAD_CONTROL_TASK_PRIORITY      11
#define RESPONSE_TRACKER_PRIORITY       10
#define VGA_DISPLAY_TASK_PRIORITY        9

#define DEFAULT_FREQ_THRESHOLD_HZ       49.0f
#define DEFAULT_ROCOF_THRESHOLD_HZPS     1.0f

#define FIRST_SHED_DEADLINE_MS         200
#define MANAGE_WINDOW_MS               500

typedef enum
{
    SYSTEM_NORMAL = 0,
    SYSTEM_MANAGING_UNSTABLE,
    SYSTEM_MANAGING_STABLE,
    SYSTEM_MAINTENANCE
} SystemMode;

typedef struct
{
    uint32_t sampleCount;
} FrequencySampleMessage;

typedef struct
{
    float frequencyHz;
    uint32_t sampleCount;
    TickType_t tickStamp;
} FrequencyMessage;

typedef struct
{
    float rocofHzPerSec;
    TickType_t tickStamp;
} ROCOFMessage;

typedef struct
{
    uint8_t buttonMask;
} ButtonMessage;

typedef enum
{
    LOAD_CMD_NONE = 0,
    LOAD_CMD_APPLY_SWITCHES,
    LOAD_CMD_SHED_NEXT,
    LOAD_CMD_RECONNECT_NEXT,
    LOAD_CMD_FORCE_REFRESH
} LoadCommandType;

typedef struct
{
    LoadCommandType type;
    TickType_t tickStamp;
} LoadCommandMessage;

typedef struct
{
    TickType_t detectionTick;
    TickType_t shedTick;
} ResponseMeasurementMessage;

typedef struct
{
    float frequencyHz;
    float rocofHzPerSec;

    float thresholdFreqHz;
    float thresholdROCOFHzPerSec;

    uint8_t switchState[LOAD_COUNT];
    uint8_t loadEnabled[LOAD_COUNT];
    uint8_t relayShed[LOAD_COUNT];

    SystemMode mode;
    uint8_t maintenanceMode;

    TickType_t lastStateChangeTick;
    TickType_t instabilityDetectedTick;
    uint8_t waitingForFirstShed;
} SharedSystemState;

extern QueueHandle_t qFrequencySamples;
extern QueueHandle_t qFrequencyToROCOF;
extern QueueHandle_t qFrequencyToDecision;
extern QueueHandle_t qROCOFToDecision;
extern QueueHandle_t qButtonToDecision;
extern QueueHandle_t qDecisionToLoadControl;
extern QueueHandle_t qResponseMeasurements;

extern SemaphoreHandle_t gStateMutex;
extern SharedSystemState gSystemState;

float absf_local(float x);

#endif
