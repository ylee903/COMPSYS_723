#include "Task_1.h"

/*
 * Temporary queue handles from Main.h / Main.c:
 * qFrequencySamples
 * qFrequencyToROCOF
 * qFrequencyToDecision
 */

/*
 * FrequencyRelayISR
 * Triggered by the hardware frequency analyser when a new sample-count
 * result is ready.
 *
 * Hardware gives:
 *   Ni = number of ADC samples between two detected peaks
 *
 * Assignment says:
 *   frequency = 16000 / Ni
 */
void FrequencyRelayISR(void *context, alt_u32 id)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    FrequencySampleMessage sampleMsg;

    (void)context;
    (void)id;

    sampleMsg.sampleCount = IORD_32DIRECT(FREQUENCY_ANALYSER_BASE, 0);

    xQueueSendFromISR(qFrequencySamples, &sampleMsg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*
 * FrequencyTask
 * Receives Ni from the ISR and converts it into frequency.
 *
 * Temporary behavior:
 * - prints frequency to Nios II Console
 * - also pushes frequency onward for later ROCOF / decision tasks
 */
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
            freqMsg.tickStamp = xTaskGetTickCount();

            /*
             * Temporary debug output:
             * view in Nios II Console / JTAG UART
             * remove or comment out later when VGA is working
             */
            printf("Ni = %lu, Frequency = %.3f Hz\n",
                   (unsigned long)freqMsg.sampleCount,
                   freqMsg.frequencyHz);

            xQueueSend(qFrequencyToROCOF, &freqMsg, 0);
            xQueueSend(qFrequencyToDecision, &freqMsg, 0);
        }
    }
}

/*
 * ROCOFTask
 * Receives frequency updates and computes ROCOF.
 *
 * Assignment brief gives the idea:
 *   ROCOF = (f_new - f_old) * sampling_frequency / average_sample_count
 *
 * Temporary behavior:
 * - prints ROCOF to console
 * - sends ROCOF onward for later DecisionTask
 */
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
                float avgSamples =
                    ((float)currentFreq.sampleCount + (float)previousFreq.sampleCount) * 0.5f;

                if (avgSamples <= 0.0f)
                {
                    avgSamples = 1.0f;
                }

                rocofMsg.rocofHzPerSec =
                    (currentFreq.frequencyHz - previousFreq.frequencyHz) * 16000.0f / avgSamples;

                rocofMsg.tickStamp = currentFreq.tickStamp;
            }

            printf("ROCOF = %.3f Hz/s\n", rocofMsg.rocofHzPerSec);

            xQueueSend(qROCOFToDecision, &rocofMsg, 0);
            previousFreq = currentFreq;
        }
    }
}
