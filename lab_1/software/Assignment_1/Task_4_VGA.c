#include "Task_4.h"

static void VGA_clear(void)
{
    int x, y;
    for (y = 0; y < VGA_CHAR_ROWS; y++)
    {
        for (x = 0; x < VGA_CHAR_COLS; x++)
        {
            IOWR_8DIRECT(
                VIDEO_CHARACTER_BUFFER_WITH_DMA_AVALON_CHAR_BUFFER_SLAVE_BASE,
                (y * VGA_CHAR_COLS) + x,
                ' '
            );
        }
    }
}

static void VGA_writeString(int col, int row, const char *text)
{
    int i = 0;
    while (text[i] != '\0')
    {
        IOWR_8DIRECT(
            VIDEO_CHARACTER_BUFFER_WITH_DMA_AVALON_CHAR_BUFFER_SLAVE_BASE,
            (row * VGA_CHAR_COLS) + col + i,
            text[i]
        );
        i++;
    }
}

void VGADisplayTask(void *pvParameters)
{
    char line[128];
    SharedSystemState localCopy;
    int i;

    (void)pvParameters;

    while (1)
    {
        xSemaphoreTake(gStateMutex, portMAX_DELAY);
        memcpy(&localCopy, &gSystemState, sizeof(localCopy));
        xSemaphoreGive(gStateMutex);

        VGA_clear();

        sprintf(line, "LCFR Assignment 1");
        VGA_writeString(0, 0, line);

        sprintf(line, "Freq: %.2f Hz", localCopy.frequencyHz);
        VGA_writeString(0, 2, line);

        sprintf(line, "ROCOF: %.2f Hz/s", localCopy.rocofHzPerSec);
        VGA_writeString(0, 3, line);

        sprintf(line, "Freq Threshold: %.2f", localCopy.thresholdFreqHz);
        VGA_writeString(0, 5, line);

        sprintf(line, "ROCOF Threshold: %.2f", localCopy.thresholdROCOFHzPerSec);
        VGA_writeString(0, 6, line);

        sprintf(line, "Mode: %s",
            (localCopy.mode == SYSTEM_NORMAL) ? "NORMAL" :
            (localCopy.mode == SYSTEM_MANAGING_UNSTABLE) ? "MANAGING_UNSTABLE" :
            (localCopy.mode == SYSTEM_MANAGING_STABLE) ? "MANAGING_STABLE" :
            "MAINTENANCE");
        VGA_writeString(0, 8, line);

        VGA_writeString(0, 10, "Loads:");
        for (i = 0; i < LOAD_COUNT; i++)
        {
            sprintf(line,
                    "L%d SW=%d ON=%d SHED=%d",
                    i,
                    localCopy.switchState[i],
                    localCopy.loadEnabled[i],
                    localCopy.relayShed[i]);
            VGA_writeString(2, 12 + i, line);
        }

        vTaskDelay(100);
    }
}
