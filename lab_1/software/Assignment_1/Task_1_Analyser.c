#include "Task_1.h"

void FrequencyRelayISR(void *context, alt_u32 id)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    FrequencySampleMessage msg;

    (void)context;
    (void)id;

    msg.sampleCount = IORD_32DIRECT(FREQUENCY_ANALYSER_BASE, 0);

    xQueueSendFromISR(qFrequencySamples, &msg, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void FrequencyTask(void *pvParameters)
{
    FrequencySampleMessage sampleMsg;
    FrequencyMessage freqMsg;

    (void)pvParameters;

    while (1)
    {
        if (xQueueReceive(qFrequencySamples, &sampleMsg, portMAX_DELAY) == pdPASS)
        {
            if (sampleMsg.sampleCount == 0)
            {
                continue;
            }

            freqMsg.sampleCount = sampleMsg.sampleCount;
            freqMsg.frequencyHz = 16000.0f / (float)sampleMsg.sampleCount;
            freqMsg.tickStamp   = xTaskGetTickCount();

            xQueueSend(qFrequencyToROCOF, &freqMsg, 0);
            xQueueSend(qFrequencyToDecision, &freqMsg, 0);
        }
    }
}

void ROCOFTask(void *pvParameters)
{
    FrequencyMessage currentFreq;
    FrequencyMessage previousFreq;
    ROCOFMessage rocofMsg;
    uint8_t havePrevious = 0;

    (void)pvParameters;

    while (1)
    {
        if (xQueueReceive(qFrequencyToROCOF, &currentFreq, portMAX_DELAY) == pdPASS)
        {
            if (!havePrevious)
            {
                previousFreq = currentFreq;
                havePrevious = 1;
                continue;
            }

            {
                float avgSamples = ((float)currentFreq.sampleCount + (float)previousFreq.sampleCount) * 0.5f;
                if (avgSamples <= 0.0f)
                {
                    avgSamples = 1.0f;
                }

                rocofMsg.rocofHzPerSec = (currentFreq.frequencyHz - previousFreq.frequencyHz) * 16000.0f / avgSamples;
                rocofMsg.tickStamp = currentFreq.tickStamp;
            }

            xQueueSend(qROCOFToDecision, &rocofMsg, 0);
            previousFreq = currentFreq;
        }
    }
}
