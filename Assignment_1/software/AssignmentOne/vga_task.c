#include <stdio.h>
#include <string.h>

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/semphr.h"
#include "shared_state.h"

#include "system.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240

/* Top graph: Frequency */
#define FREQ_GRAPH_X0       30
#define FREQ_GRAPH_Y0       12
#define FREQ_GRAPH_WIDTH    270
#define FREQ_GRAPH_HEIGHT   70

/* Bottom graph: ROCOF */
#define ROCOF_GRAPH_X0      30
#define ROCOF_GRAPH_Y0      98
#define ROCOF_GRAPH_WIDTH   270
#define ROCOF_GRAPH_HEIGHT  50

#define HISTORY_LEN         FREQ_GRAPH_WIDTH

/* Simple 5x7 font for ASCII 32-126 */
const unsigned char font5x7[95][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},
    {0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},{0x00,0x1C,0x22,0x41,0x00},
    {0x00,0x41,0x22,0x1C,0x00},{0x14,0x08,0x3E,0x08,0x14},{0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02},{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},
    {0x00,0x56,0x36,0x00,0x00},{0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},{0x32,0x49,0x79,0x41,0x3E},
    {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},{0x00,0x7F,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x41,0x7F,0x00},{0x04,0x02,0x01,0x02,0x04},
    {0x40,0x40,0x40,0x40,0x40},{0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},
    {0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},{0x38,0x44,0x44,0x48,0x7F},
    {0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x0C,0x52,0x52,0x52,0x3E},
    {0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},
    {0x7F,0x10,0x28,0x44,0x00},{0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x18,0x04,0x78},
    {0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},{0x7C,0x14,0x14,0x14,0x08},
    {0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},
    {0x3C,0x40,0x30,0x40,0x3C},{0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},
    {0x44,0x64,0x54,0x4C,0x44},{0x00,0x08,0x36,0x41,0x00},{0x00,0x00,0x7F,0x00,0x00},
    {0x00,0x41,0x36,0x08,0x00},{0x10,0x08,0x08,0x10,0x08}
};

static alt_up_pixel_buffer_dma_dev *pixel_buffer_dev = NULL;
static float freqHistory[HISTORY_LEN];
static float rocofHistory[HISTORY_LEN];
static int historyIndex = 0;
static int historyFilled = 0;

static void plot_pixel(int x, int y, short colour)
{
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
    {
        alt_up_pixel_buffer_dma_draw(pixel_buffer_dev, colour, x, y);
    }
}

static void draw_line(int x1, int y1, int x2, int y2, short colour)
{
    alt_up_pixel_buffer_dma_draw_line(pixel_buffer_dev, x1, y1, x2, y2, colour, 0);
}

static void clear_screen(void)
{
    alt_up_pixel_buffer_dma_clear_screen(pixel_buffer_dev, 0);
}

static void draw_char(int x, int y, char c, short colour)
{
    int i, j;
    unsigned char line;

    if (c < 32 || c > 126) return;

    for (i = 0; i < 5; i++)
    {
        line = font5x7[c - 32][i];
        for (j = 0; j < 7; j++)
        {
            if (line & (1 << j))
            {
                plot_pixel(x + i, y + j, colour);
            }
        }
    }
}

static void draw_string(int x, int y, const char *str, short colour)
{
    while (*str)
    {
        draw_char(x, y, *str, colour);
        x += 6;
        str++;
    }
}

static int map_freq_to_y(float freq)
{
    float minFreq = 45.0f;
    float maxFreq = 55.0f;

    if (freq < minFreq) freq = minFreq;
    if (freq > maxFreq) freq = maxFreq;

    return FREQ_GRAPH_Y0 + FREQ_GRAPH_HEIGHT
         - (int)(((freq - minFreq) / (maxFreq - minFreq)) * FREQ_GRAPH_HEIGHT);
}

static int map_rocof_to_y(float rocof)
{
    float minR = -30.0f;
    float maxR = 30.0f;

    if (rocof < minR) rocof = minR;
    if (rocof > maxR) rocof = maxR;

    return ROCOF_GRAPH_Y0 + ROCOF_GRAPH_HEIGHT
         - (int)(((rocof - minR) / (maxR - minR)) * ROCOF_GRAPH_HEIGHT);
}

static void draw_freq_axes_and_threshold(float freqThreshold)
{
    int yThresh;

    draw_line(FREQ_GRAPH_X0, FREQ_GRAPH_Y0,
              FREQ_GRAPH_X0, FREQ_GRAPH_Y0 + FREQ_GRAPH_HEIGHT, 0x7BEF);
    draw_line(FREQ_GRAPH_X0, FREQ_GRAPH_Y0 + FREQ_GRAPH_HEIGHT,
              FREQ_GRAPH_X0 + FREQ_GRAPH_WIDTH, FREQ_GRAPH_Y0 + FREQ_GRAPH_HEIGHT, 0x7BEF);

    draw_line(FREQ_GRAPH_X0 - 4, FREQ_GRAPH_Y0,
              FREQ_GRAPH_X0, FREQ_GRAPH_Y0, 0x7BEF);
    draw_line(FREQ_GRAPH_X0 - 4, FREQ_GRAPH_Y0 + FREQ_GRAPH_HEIGHT / 2,
              FREQ_GRAPH_X0, FREQ_GRAPH_Y0 + FREQ_GRAPH_HEIGHT / 2, 0x7BEF);
    draw_line(FREQ_GRAPH_X0 - 4, FREQ_GRAPH_Y0 + FREQ_GRAPH_HEIGHT,
              FREQ_GRAPH_X0, FREQ_GRAPH_Y0 + FREQ_GRAPH_HEIGHT, 0x7BEF);

    yThresh = map_freq_to_y(freqThreshold);
    draw_line(FREQ_GRAPH_X0, yThresh,
              FREQ_GRAPH_X0 + FREQ_GRAPH_WIDTH, yThresh, 0xF800);

    draw_string(2, FREQ_GRAPH_Y0 - 2, "Hz", 0xFFFF);
    draw_string(2, FREQ_GRAPH_Y0 - 2 + 12, "55", 0xFFFF);
    draw_string(2, FREQ_GRAPH_Y0 + FREQ_GRAPH_HEIGHT / 2 - 3, "50", 0xFFFF);
    draw_string(2, FREQ_GRAPH_Y0 + FREQ_GRAPH_HEIGHT - 6, "45", 0xFFFF);
    draw_string(FREQ_GRAPH_X0 + FREQ_GRAPH_WIDTH / 2 - 12,
                FREQ_GRAPH_Y0 + FREQ_GRAPH_HEIGHT + 8, "Time", 0xFFFF);
}

static void draw_rocof_axes_and_threshold(float rocofThreshold)
{
    int yZero, yTopThresh, yBotThresh;

    draw_line(ROCOF_GRAPH_X0, ROCOF_GRAPH_Y0,
              ROCOF_GRAPH_X0, ROCOF_GRAPH_Y0 + ROCOF_GRAPH_HEIGHT, 0x7BEF);
    draw_line(ROCOF_GRAPH_X0, ROCOF_GRAPH_Y0 + ROCOF_GRAPH_HEIGHT,
              ROCOF_GRAPH_X0 + ROCOF_GRAPH_WIDTH, ROCOF_GRAPH_Y0 + ROCOF_GRAPH_HEIGHT, 0x7BEF);

    yZero = map_rocof_to_y(0.0f);
    draw_line(ROCOF_GRAPH_X0, yZero,
              ROCOF_GRAPH_X0 + ROCOF_GRAPH_WIDTH, yZero, 0x7BEF);

    yTopThresh = map_rocof_to_y(rocofThreshold);
    yBotThresh = map_rocof_to_y(-rocofThreshold);

    draw_line(ROCOF_GRAPH_X0, yTopThresh,
              ROCOF_GRAPH_X0 + ROCOF_GRAPH_WIDTH, yTopThresh, 0xF800);
    draw_line(ROCOF_GRAPH_X0, yBotThresh,
              ROCOF_GRAPH_X0 + ROCOF_GRAPH_WIDTH, yBotThresh, 0xF800);

    draw_string(2, ROCOF_GRAPH_Y0 - 2, "dfdt", 0xFFFF);
    draw_string(2, ROCOF_GRAPH_Y0 - 2 + 10, "30", 0xFFFF);
    draw_string(2, yZero - 3, "0", 0xFFFF);
    draw_string(2, ROCOF_GRAPH_Y0 + ROCOF_GRAPH_HEIGHT - 6, "-30", 0xFFFF);
}

static void draw_frequency_trace(void)
{
    int count, i;
    int idx1, idx2;
    int x1, x2;
    int y1, y2;

    count = historyFilled ? HISTORY_LEN : historyIndex;
    if (count < 2) return;

    for (i = 0; i < count - 1; i++)
    {
        idx1 = (historyFilled ? historyIndex : 0) + i;
        idx2 = idx1 + 1;

        idx1 %= HISTORY_LEN;
        idx2 %= HISTORY_LEN;

        x1 = FREQ_GRAPH_X0 + i;
        x2 = FREQ_GRAPH_X0 + i + 1;

        y1 = map_freq_to_y(freqHistory[idx1]);
        y2 = map_freq_to_y(freqHistory[idx2]);

        draw_line(x1, y1, x2, y2, 0x001F);
    }
}

static void draw_rocof_trace(void)
{
    int count, i;
    int idx1, idx2;
    int x1, x2;
    int y1, y2;

    count = historyFilled ? HISTORY_LEN : historyIndex;
    if (count < 2) return;

    for (i = 0; i < count - 1; i++)
    {
        idx1 = (historyFilled ? historyIndex : 0) + i;
        idx2 = idx1 + 1;

        idx1 %= HISTORY_LEN;
        idx2 %= HISTORY_LEN;

        x1 = ROCOF_GRAPH_X0 + i;
        x2 = ROCOF_GRAPH_X0 + i + 1;

        y1 = map_rocof_to_y(rocofHistory[idx1]);
        y2 = map_rocof_to_y(rocofHistory[idx2]);

        draw_line(x1, y1, x2, y2, 0x001F);
    }
}

void VGATask(void *pvParameters)
{
    float freq, rocof, freqThresh, rocofThresh;
    unsigned int unstable, managing, loads, maintenance;
    unsigned int recent[5], recCount, recentIdx;
    unsigned int minT, maxT, totalT, measCount;
    TickType_t startTick;
    char buffer[64];
    int y;

    pixel_buffer_dev = alt_up_pixel_buffer_dma_open_dev(VIDEO_PIXEL_BUFFER_DMA_NAME);
    if (pixel_buffer_dev == NULL)
    {
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    clear_screen();

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

        freqHistory[historyIndex] = freq;
        rocofHistory[historyIndex] = rocof;
        historyIndex++;

        if (historyIndex >= HISTORY_LEN)
        {
            historyIndex = 0;
            historyFilled = 1;
        }

        clear_screen();

        draw_freq_axes_and_threshold(freqThresh);
        draw_frequency_trace();

        draw_rocof_axes_and_threshold(rocofThresh);
        draw_rocof_trace();

        y = 160;

        sprintf(buffer, "Freq: %.2f Hz", freq);
        draw_string(10, y, buffer, 0xFFFF);
        y += 10;

        sprintf(buffer, "ROCOF: %.2f Hz/s", rocof);
        draw_string(10, y, buffer, 0xFFFF);
        y += 10;

        sprintf(buffer, "State: %s / %s / %s",
                maintenance ? "Maint" : "Run",
                unstable ? "Unstable" : "Stable",
                managing ? "Managing" : "Normal");
        draw_string(10, y, buffer, 0xFFFF);
        y += 10;

        sprintf(buffer, "Loads: ");
        {
            int len = (int)strlen(buffer);
            int i;
            for (i = 0; i < 5; i++)
            {
                buffer[len++] = (loads & (1U << i)) ? '1' : '0';
                buffer[len++] = ' ';
            }
            buffer[len] = '\0';
        }
        draw_string(10, y, buffer, 0xFFFF);
        y += 10;

        sprintf(buffer, "Thres F %.1f  R %.1f", freqThresh, rocofThresh);
        draw_string(10, y, buffer, 0xFFFF);
        y += 10;

        sprintf(buffer, "Recent:");
        draw_string(10, y, buffer, 0xFFFF);
        {
            int i;
            for (i = 0; i < (int)recCount; i++)
            {
                sprintf(buffer, "%u", recent[(recentIdx + 5 - recCount + i) % 5]);
                draw_string(60 + i * 34, y, buffer, 0xFFFF);
            }
        }
        y += 10;

        sprintf(buffer, "Min:%u Max:%u", minT == 0xFFFFFFFF ? 0 : minT, maxT);
        draw_string(10, y, buffer, 0xFFFF);
        y += 10;

        sprintf(buffer, "Avg:%u",
                (measCount > 0) ? (totalT / measCount) : 0);
        draw_string(10, y, buffer, 0xFFFF);

        {
            TickType_t now = xTaskGetTickCount();
            unsigned int activeMs =
                (unsigned int)((now - startTick) * portTICK_PERIOD_MS);

            sprintf(buffer, "Active:%u ms", activeMs);
            draw_string(120, y, buffer, 0xFFFF);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
