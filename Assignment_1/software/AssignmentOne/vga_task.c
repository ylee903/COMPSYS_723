#include <stdio.h>
#include <string.h>

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/semphr.h"
#include "shared_state.h"

#include "system.h"
#include "io.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"

/* Based on provided graph design */
#define FREQPLT_ORI_X       101
#define FREQPLT_GRID_SIZE_X 5
#define FREQPLT_ORI_Y       199.0
#define FREQPLT_FREQ_RES    20.0

#define ROCPLT_ORI_X        101
#define ROCPLT_GRID_SIZE_X  5
#define ROCPLT_ORI_Y        259.0
#define ROCPLT_ROC_RES      0.5

#define MIN_FREQ            45.0
#define MAX_FREQ            55.0

#define HISTORY_POINTS      100

typedef struct {
    unsigned int x1;
    unsigned int y1;
    unsigned int x2;
    unsigned int y2;
} Line;

void VGATask(void *pvParameters)
{
    alt_up_pixel_buffer_dma_dev *pixel_buf;
    alt_up_char_buffer_dev *char_buf;

    float freqHistory[HISTORY_POINTS];
    float rocHistory[HISTORY_POINTS];
    int histIndex = 99;
    int j;

    float freq, rocof, freqThresh, rocofThresh;
    unsigned int unstable, managing, loads, maintenance;
    unsigned int recent[5], recCount, recentIdx;
    unsigned int minT, maxT, totalT, measCount;
    TickType_t startTick;

    char buffer[64];
    Line line_freq, line_roc;

    pixel_buf = alt_up_pixel_buffer_dma_open_dev(VIDEO_PIXEL_BUFFER_DMA_NAME);
    if (pixel_buf == NULL)
    {
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    /* Change this name if your system.h uses a different char buffer name */
    char_buf = alt_up_char_buffer_open_dev("/dev/video_character_buffer_with_dma");
    if (char_buf == NULL)
    {
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    alt_up_pixel_buffer_dma_clear_screen(pixel_buf, 0);
    alt_up_char_buffer_clear(char_buf);

    /* Static axes and labels, following provided design */
    alt_up_pixel_buffer_dma_draw_hline(pixel_buf, 100, 590, 200, 0xFFFF, 0);
    alt_up_pixel_buffer_dma_draw_hline(pixel_buf, 100, 590, 300, 0xFFFF, 0);
    alt_up_pixel_buffer_dma_draw_vline(pixel_buf, 100, 50, 200, 0xFFFF, 0);
    alt_up_pixel_buffer_dma_draw_vline(pixel_buf, 100, 220, 300, 0xFFFF, 0);

    alt_up_char_buffer_string(char_buf, "Frequency (Hz)", 4, 4);
    alt_up_char_buffer_string(char_buf, "55", 10, 7);
    alt_up_char_buffer_string(char_buf, "50", 10, 12);
    alt_up_char_buffer_string(char_buf, "45", 10, 17);

    alt_up_char_buffer_string(char_buf, "Rate of Change (dF/dt Hz/s)", 3, 23);
    alt_up_char_buffer_string(char_buf, "60", 10, 28);
    alt_up_char_buffer_string(char_buf, "30", 10, 30);
    alt_up_char_buffer_string(char_buf, "0", 10, 32);
    alt_up_char_buffer_string(char_buf, "-30", 9, 34);
    alt_up_char_buffer_string(char_buf, "-60", 9, 36);

    /* --- Top row --- */
    alt_up_char_buffer_string(char_buf, "System:", 5, 2);
    alt_up_char_buffer_string(char_buf, "Loads:", 30, 2);
    alt_up_char_buffer_string(char_buf, "Active:", 55, 2);

    alt_up_char_buffer_string(char_buf, "Freq Threshold:", 5, 41);
    alt_up_char_buffer_string(char_buf, "RoC Threshold:", 5, 43);

    /* --- Measurements (your existing block) --- */
    alt_up_char_buffer_string(char_buf, "Average Time:", 5, 45);
    alt_up_char_buffer_string(char_buf, "Max Time:", 5, 47);
    alt_up_char_buffer_string(char_buf, "Min Time:", 5, 49);
    alt_up_char_buffer_string(char_buf, "Last 5 Values:", 30, 45);

    for (j = 0; j < HISTORY_POINTS; j++)
    {
        freqHistory[j] = 50.0f;
        rocHistory[j] = 0.0f;
    }

    while (1)
    {
        if (xSemaphoreTake(sharedStateMutex, portMAX_DELAY) == pdTRUE)
        {
            freq = currentFrequencyHz;
            rocof = currentROCOF;
            freqThresh = frequencyThreshold;
            rocofThresh = rocofThreshold;
            unstable = systemUnstable;
            managing = managingLoads;
            loads = actualLoads;
            maintenance = maintenanceMode;

            memcpy(recent, recentTimes, sizeof(recentTimes));
            recCount = recentCount;
            recentIdx = recentIndex;
            minT = minTime;
            maxT = maxTime;
            totalT = totalTime;
            measCount = measurementCount;
            startTick = systemStartTick;

            xSemaphoreGive(sharedStateMutex);
        }

        /* Update rolling history, same style as provided design */
        freqHistory[histIndex] = freq;
        rocHistory[histIndex] = rocof;
        histIndex = ++histIndex % HISTORY_POINTS;

        /* Clear old graph areas only */
        alt_up_pixel_buffer_dma_draw_box(pixel_buf, 101, 51, 639, 199, 0, 0);
        alt_up_pixel_buffer_dma_draw_box(pixel_buf, 101, 221, 639, 299, 0, 0);

        /* Draw threshold lines */
        {
            int yFreqThresh = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (freqThresh - MIN_FREQ));
            int yRocTop = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * rocofThresh);
            int yRocBot = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * (-rocofThresh));

            alt_up_pixel_buffer_dma_draw_line(pixel_buf, 101, yFreqThresh, 590, yFreqThresh, 0xF800, 0);
            alt_up_pixel_buffer_dma_draw_line(pixel_buf, 101, yRocTop, 590, yRocTop, 0xF800, 0);
            alt_up_pixel_buffer_dma_draw_line(pixel_buf, 101, yRocBot, 590, yRocBot, 0xF800, 0);
        }

        /* Draw frequency and ROCOF graphs */
        for (j = 0; j < HISTORY_POINTS - 1; ++j)
        {
            /* Frequency */
            line_freq.x1 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * j;
            line_freq.y1 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES *
                                 (freqHistory[(histIndex + j) % HISTORY_POINTS] - MIN_FREQ));

            line_freq.x2 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * (j + 1);
            line_freq.y2 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES *
                                 (freqHistory[(histIndex + j + 1) % HISTORY_POINTS] - MIN_FREQ));

            /* ROCOF */
            line_roc.x1 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * j;
            line_roc.y1 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES *
                                rocHistory[(histIndex + j) % HISTORY_POINTS]);

            line_roc.x2 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * (j + 1);
            line_roc.y2 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES *
                                rocHistory[(histIndex + j + 1) % HISTORY_POINTS]);

            alt_up_pixel_buffer_dma_draw_line(pixel_buf,
                                              line_freq.x1, line_freq.y1,
                                              line_freq.x2, line_freq.y2,
                                              0x001F, 0);

            alt_up_pixel_buffer_dma_draw_line(pixel_buf,
                                              line_roc.x1, line_roc.y1,
                                              line_roc.x2, line_roc.y2,
                                              0x001F, 0);
        }

        /* Update text fields */

        /* --- Top row values --- */
        if (maintenance)
        {
            alt_up_char_buffer_string(char_buf, "Maint      ", 13, 2);
        }
        else if (managing)
        {
            alt_up_char_buffer_string(char_buf, "Managing   ", 13, 2);
        }
        else if (unstable)
        {
            alt_up_char_buffer_string(char_buf, "Unstable   ", 13, 2);
        }
        else
        {
            alt_up_char_buffer_string(char_buf, "Stable     ", 13, 2);
        }

        sprintf(buffer, "[%c][%c][%c][%c][%c]   ",
                (loads & (1U << 0)) ? '1' : '0',
                (loads & (1U << 1)) ? '1' : '0',
                (loads & (1U << 2)) ? '1' : '0',
                (loads & (1U << 3)) ? '1' : '0',
                (loads & (1U << 4)) ? '1' : '0');
        alt_up_char_buffer_string(char_buf, buffer, 37, 2);

        {
            unsigned int activeSec =
                (unsigned int)((xTaskGetTickCount() - startTick) * portTICK_PERIOD_MS / 1000U);
            sprintf(buffer, "%u s      ", activeSec);
            alt_up_char_buffer_string(char_buf, buffer, 63, 2);
        }

        /* --- Bottom threshold block --- */
        sprintf(buffer, "%5.2f Hz   ", freqThresh);
        alt_up_char_buffer_string(char_buf, buffer, 20, 41);

        sprintf(buffer, "%5.2f Hz/s ", rocofThresh);
        alt_up_char_buffer_string(char_buf, buffer, 20, 43);

        /* --- Bottom measurement block --- */
        sprintf(buffer, "%u ms   ", (measCount > 0) ? (totalT / measCount) : 0);
        alt_up_char_buffer_string(char_buf, buffer, 20, 45);

        sprintf(buffer, "%u ms   ", maxT == 0 ? 0 : maxT);
        alt_up_char_buffer_string(char_buf, buffer, 16, 47);

        sprintf(buffer, "%u ms   ", (minT == 0xFFFFFFFF) ? 0 : minT);
        alt_up_char_buffer_string(char_buf, buffer, 16, 49);

        sprintf(buffer, "%u %u %u %u %u      ",
                recent[(recentIdx + 0) % 5],
                recent[(recentIdx + 1) % 5],
                recent[(recentIdx + 2) % 5],
                recent[(recentIdx + 3) % 5],
                recent[(recentIdx + 4) % 5]);
        alt_up_char_buffer_string(char_buf, buffer, 48, 45);

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
