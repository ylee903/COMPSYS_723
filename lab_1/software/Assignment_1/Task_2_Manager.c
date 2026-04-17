#include "Task_2.h"

static int findLowestPriorityOnLoad(const SharedSystemState *state)
{
    int i;
    for (i = 0; i < LOAD_COUNT; i++)
    {
        if (state->loadEnabled[i])
        {
            return i;
        }
    }
    return -1;
}

static int findHighestPriorityShedLoad(const SharedSystemState *state)
{
    int i;
    for (i = LOAD_COUNT - 1; i >= 0; i--)
    {
        if (state->relayShed[i])
        {
            return i;
        }
    }
    return -1;
}

static void refreshSwitchSnapshot(SharedSystemState *state)
{
    uint32_t raw = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
    int i;

    for (i = 0; i < LOAD_COUNT; i++)
    {
        state->switchState[i] = (raw >> i) & 0x1u;
    }
}

void KeyboardISR(void *context, alt_u32 id)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    KeyboardMessage msg;

    (void)context;
    (void)id;

    /* Minimal starter: raw PS/2 register byte */
    msg.rawByte = (uint8_t)(IORD_32DIRECT(PS2_BASE, 0) & 0xFFu);

    xQueueSendFromISR(qKeyboardToTask, &msg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void PushButtonISR(void *context, alt_u32 id)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    ButtonMessage msg;

    (void)context;
    (void)id;

    msg.buttonMask = IORD_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE);
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0);

    xQueueSendFromISR(qButtonToDecision, &msg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void KeyboardTask(void *pvParameters)
{
    KeyboardMessage msg;

    (void)pvParameters;

    while (1)
    {
        if (xQueueReceive(qKeyboardToTask, &msg, portMAX_DELAY) == pdPASS)
        {
            /*
             * Starter only:
             * q = lower frequency threshold
             * w = higher frequency threshold
             * a = lower ROCOF threshold
             * s = higher ROCOF threshold
             */
            xSemaphoreTake(gStateMutex, portMAX_DELAY);

            switch (msg.rawByte)
            {
                case 'q':
                    gSystemState.thresholdFreqHz -= 0.1f;
                    break;
                case 'w':
                    gSystemState.thresholdFreqHz += 0.1f;
                    break;
                case 'a':
                    gSystemState.thresholdROCOFHzPerSec -= 0.1f;
                    if (gSystemState.thresholdROCOFHzPerSec < 0.1f)
                        gSystemState.thresholdROCOFHzPerSec = 0.1f;
                    break;
                case 's':
                    gSystemState.thresholdROCOFHzPerSec += 0.1f;
                    break;
                default:
                    break;
            }

            xSemaphoreGive(gStateMutex);
        }
    }
}

void DecisionTask(void *pvParameters)
{
    FrequencyMessage freqMsg;
    ROCOFMessage rocofMsg;
    ButtonMessage buttonMsg;
    LoadCommandMessage cmd;

    float latestFreq = 50.0f;
    float latestROCOF = 0.0f;
    uint8_t haveFreq = 0;
    uint8_t haveROCOF = 0;

    (void)pvParameters;

    while (1)
    {
        while (xQueueReceive(qFrequencyToDecision, &freqMsg, 0) == pdPASS)
        {
            latestFreq = freqMsg.frequencyHz;
            haveFreq = 1;
        }

        while (xQueueReceive(qROCOFToDecision, &rocofMsg, 0) == pdPASS)
        {
            latestROCOF = rocofMsg.rocofHzPerSec;
            haveROCOF = 1;
        }

        while (xQueueReceive(qButtonToDecision, &buttonMsg, 0) == pdPASS)
        {
            (void)buttonMsg;
            xSemaphoreTake(gStateMutex, portMAX_DELAY);
            gSystemState.maintenanceMode ^= 1u;
            gSystemState.mode = gSystemState.maintenanceMode ? SYSTEM_MAINTENANCE : SYSTEM_NORMAL;
            gSystemState.lastStateChangeTick = xTaskGetTickCount();
            xSemaphoreGive(gStateMutex);

            cmd.type = LOAD_CMD_FORCE_REFRESH;
            cmd.tickStamp = xTaskGetTickCount();
            xQueueSend(qDecisionToLoadControl, &cmd, 0);
        }

        if (haveFreq)
        {
            xSemaphoreTake(gStateMutex, portMAX_DELAY);
            gSystemState.frequencyHz = latestFreq;
            if (haveROCOF)
                gSystemState.rocofHzPerSec = latestROCOF;
            xSemaphoreGive(gStateMutex);
        }

        xSemaphoreTake(gStateMutex, portMAX_DELAY);

        refreshSwitchSnapshot(&gSystemState);

        if (gSystemState.maintenanceMode)
        {
            gSystemState.mode = SYSTEM_MAINTENANCE;

            cmd.type = LOAD_CMD_APPLY_SWITCHES;
            cmd.tickStamp = xTaskGetTickCount();
            xQueueSend(qDecisionToLoadControl, &cmd, 0);

            xSemaphoreGive(gStateMutex);
            vTaskDelay(20);
            continue;
        }

        {
            uint8_t unstable =
                (gSystemState.frequencyHz < gSystemState.thresholdFreqHz) ||
                (absf_local(gSystemState.rocofHzPerSec) > gSystemState.thresholdROCOFHzPerSec);

            TickType_t now = xTaskGetTickCount();

            if (unstable)
            {
                if (gSystemState.mode == SYSTEM_NORMAL)
                {
                    gSystemState.mode = SYSTEM_MANAGING_UNSTABLE;
                    gSystemState.lastStateChangeTick = now;
                    gSystemState.instabilityDetectedTick = now;
                    gSystemState.waitingForFirstShed = 1;

                    cmd.type = LOAD_CMD_SHED_NEXT;
                    cmd.tickStamp = now;
                    xQueueSend(qDecisionToLoadControl, &cmd, 0);
                }
                else if (gSystemState.mode == SYSTEM_MANAGING_STABLE)
                {
                    gSystemState.mode = SYSTEM_MANAGING_UNSTABLE;
                    gSystemState.lastStateChangeTick = now;
                }
                else if (gSystemState.mode == SYSTEM_MANAGING_UNSTABLE)
                {
                    if ((now - gSystemState.lastStateChangeTick) >= MANAGE_WINDOW_MS)
                    {
                        cmd.type = LOAD_CMD_SHED_NEXT;
                        cmd.tickStamp = now;
                        xQueueSend(qDecisionToLoadControl, &cmd, 0);
                        gSystemState.lastStateChangeTick = now;
                    }
                }
            }
            else
            {
                if (gSystemState.mode == SYSTEM_MANAGING_UNSTABLE)
                {
                    gSystemState.mode = SYSTEM_MANAGING_STABLE;
                    gSystemState.lastStateChangeTick = now;
                }
                else if (gSystemState.mode == SYSTEM_MANAGING_STABLE)
                {
                    if ((now - gSystemState.lastStateChangeTick) >= MANAGE_WINDOW_MS)
                    {
                        cmd.type = LOAD_CMD_RECONNECT_NEXT;
                        cmd.tickStamp = now;
                        xQueueSend(qDecisionToLoadControl, &cmd, 0);
                        gSystemState.lastStateChangeTick = now;
                    }
                }
                else if (gSystemState.mode == SYSTEM_NORMAL)
                {
                    cmd.type = LOAD_CMD_APPLY_SWITCHES;
                    cmd.tickStamp = now;
                    xQueueSend(qDecisionToLoadControl, &cmd, 0);
                }
            }
        }

        xSemaphoreGive(gStateMutex);
        vTaskDelay(10);
    }
}

void LoadControlTask(void *pvParameters)
{
    LoadCommandMessage cmd;
    ResponseMeasurementMessage measurement;

    (void)pvParameters;

    while (1)
    {
        if (xQueueReceive(qDecisionToLoadControl, &cmd, portMAX_DELAY) == pdPASS)
        {
            xSemaphoreTake(gStateMutex, portMAX_DELAY);

            refreshSwitchSnapshot(&gSystemState);

            if (gSystemState.maintenanceMode)
            {
                int i;
                for (i = 0; i < LOAD_COUNT; i++)
                {
                    gSystemState.loadEnabled[i] = gSystemState.switchState[i];
                    gSystemState.relayShed[i] = 0;
                }
            }
            else
            {
                if (cmd.type == LOAD_CMD_APPLY_SWITCHES || cmd.type == LOAD_CMD_FORCE_REFRESH)
                {
                    int i;
                    for (i = 0; i < LOAD_COUNT; i++)
                    {
                        if (!gSystemState.relayShed[i])
                        {
                            gSystemState.loadEnabled[i] = gSystemState.switchState[i];
                        }
                        else
                        {
                            if (gSystemState.switchState[i] == 0)
                            {
                                gSystemState.loadEnabled[i] = 0;
                                gSystemState.relayShed[i] = 0;
                            }
                        }
                    }
                }
                else if (cmd.type == LOAD_CMD_SHED_NEXT)
                {
                    int idx = findLowestPriorityOnLoad(&gSystemState);
                    if (idx >= 0)
                    {
                        gSystemState.loadEnabled[idx] = 0;
                        gSystemState.relayShed[idx] = 1;

                        if (gSystemState.waitingForFirstShed)
                        {
                            measurement.detectionTick = gSystemState.instabilityDetectedTick;
                            measurement.shedTick = xTaskGetTickCount();
                            xQueueSend(qResponseMeasurements, &measurement, 0);
                            gSystemState.waitingForFirstShed = 0;
                        }
                    }
                }
                else if (cmd.type == LOAD_CMD_RECONNECT_NEXT)
                {
                    int idx = findHighestPriorityShedLoad(&gSystemState);
                    if (idx >= 0)
                    {
                        gSystemState.relayShed[idx] = 0;
                        gSystemState.loadEnabled[idx] = gSystemState.switchState[idx];
                    }

                    {
                        int anyShed = 0;
                        int i;
                        for (i = 0; i < LOAD_COUNT; i++)
                        {
                            if (gSystemState.relayShed[i])
                            {
                                anyShed = 1;
                                break;
                            }
                        }
                        if (!anyShed)
                        {
                            gSystemState.mode = SYSTEM_NORMAL;
                        }
                    }
                }
            }

            IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE,   packRedLEDsFromState(&gSystemState));
            IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, packGreenLEDsFromState(&gSystemState));

            xSemaphoreGive(gStateMutex);
        }
    }
}
