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

#define GRAPH_X0        30
#define GRAPH_Y0        20
#define GRAPH_WIDTH     260
#define GRAPH_HEIGHT    70

#define HISTORY_LEN     GRAPH_WIDTH

// Simple 5x7 font for ASCII 32-126
const unsigned char font5x7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 backslash
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 }
    {0x10, 0x08, 0x08, 0x10, 0x08}, // 126 ~
};

static alt_up_pixel_buffer_dma_dev *pixel_buffer_dev = NULL;
static float freqHistory[HISTORY_LEN];
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
    for (i = 0; i < 5; i++) {
        line = font5x7[c - 32][i];
        for (j = 0; j < 7; j++) {
            if (line & (1 << j)) {
                plot_pixel(x + i, y + j, colour);
            }
        }
    }
}

static void draw_string(int x, int y, const char* str, short colour)
{
    while (*str) {
        draw_char(x, y, *str, colour);
        x += 6; // 5 pixels + 1 space
        str++;
    }
}

static int map_freq_to_y(float freq)
{
    float minFreq = 45.0f;
    float maxFreq = 55.0f;

    if (freq < minFreq) freq = minFreq;
    if (freq > maxFreq) freq = maxFreq;

    return GRAPH_Y0 + GRAPH_HEIGHT
         - (int)(((freq - minFreq) / (maxFreq - minFreq)) * GRAPH_HEIGHT);
}

static void draw_axes_and_threshold(float freqThreshold)
{
    int yThresh;

    /* Axes */
    draw_line(GRAPH_X0, GRAPH_Y0, GRAPH_X0, GRAPH_Y0 + GRAPH_HEIGHT, 0x7BEF);
    draw_line(GRAPH_X0, GRAPH_Y0 + GRAPH_HEIGHT, GRAPH_X0 + GRAPH_WIDTH, GRAPH_Y0 + GRAPH_HEIGHT, 0x7BEF);

    /* Small y-axis ticks */
    draw_line(GRAPH_X0 - 5, GRAPH_Y0, GRAPH_X0, GRAPH_Y0, 0x7BEF);
    draw_line(GRAPH_X0 - 5, GRAPH_Y0 + GRAPH_HEIGHT / 2, GRAPH_X0, GRAPH_Y0 + GRAPH_HEIGHT / 2, 0x7BEF);
    draw_line(GRAPH_X0 - 5, GRAPH_Y0 + GRAPH_HEIGHT, GRAPH_X0, GRAPH_Y0 + GRAPH_HEIGHT, 0x7BEF);

    /* Threshold line */
    yThresh = map_freq_to_y(freqThreshold);
    draw_line(GRAPH_X0, yThresh, GRAPH_X0 + GRAPH_WIDTH, yThresh, 0xF800); /* red */
}

static void draw_frequency_trace(void)
{
    int count;
    int i;
    int idx1, idx2;
    int x1, x2;
    int y1, y2;

    count = historyFilled ? HISTORY_LEN : historyIndex;

    if (count < 2)
        return;

    for (i = 0; i < count - 1; i++)
    {
        idx1 = (historyFilled ? historyIndex : 0) + i;
        idx2 = idx1 + 1;

        idx1 %= HISTORY_LEN;
        idx2 %= HISTORY_LEN;

        x1 = GRAPH_X0 + i;
        x2 = GRAPH_X0 + i + 1;

        y1 = map_freq_to_y(freqHistory[idx1]);
        y2 = map_freq_to_y(freqHistory[idx2]);

        draw_line(x1, y1, x2, y2, 0x001F); /* blue */
    }
}

void VGATask(void *pvParameters)
{
    float freq, rocof, freqThresh, rocofThresh;
    unsigned int unstable, managing, loads, maintenance;
    unsigned int recent[5], recCount, minT, maxT, totalT, measCount;
    TickType_t startTick;
    char buffer[64];

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
            minT = minTime;
            maxT = maxTime;
            totalT = totalTime;
            measCount = measurementCount;
            startTick = systemStartTick;
            xSemaphoreGive(sharedStateMutex);
        }

        freqHistory[historyIndex] = freq;
        historyIndex++;

        if (historyIndex >= HISTORY_LEN)
        {
            historyIndex = 0;
            historyFilled = 1;
        }

        clear_screen();
        draw_axes_and_threshold(freqThresh);
        draw_frequency_trace();

        // Draw text information
        int y = 100;
        sprintf(buffer, "Freq: %.2f Hz", freq);
        draw_string(10, y, buffer, 0xFFFF); y += 10;

        sprintf(buffer, "ROCOF: %.2f Hz/s", rocof);
        draw_string(10, y, buffer, 0xFFFF); y += 10;

        sprintf(buffer, "State: %s / %s", unstable ? "Unstable" : "Stable", managing ? "Managing" : "Normal");
        draw_string(10, y, buffer, 0xFFFF); y += 10;

        sprintf(buffer, "Loads: ");
        int len = strlen(buffer);
        for (int i = 0; i < 5; i++) {
            buffer[len++] = (loads & (1 << i)) ? '1' : '0';
            buffer[len++] = ' ';
        }
        buffer[len] = '\0';
        draw_string(10, y, buffer, 0xFFFF); y += 10;

        sprintf(buffer, "Thresh: Freq %.1f, ROCOF %.1f", freqThresh, rocofThresh);
        draw_string(10, y, buffer, 0xFFFF); y += 10;

        // Statistics
        y += 10;
        sprintf(buffer, "Recent:");
        draw_string(10, y, buffer, 0xFFFF);
        for (int i = 0; i < recCount; i++) {
            sprintf(buffer, " %u", recent[(recentIndex + 5 - recCount + i) % 5]);
            draw_string(10 + 60 + i*40, y, buffer, 0xFFFF);
        }
        y += 10;

        sprintf(buffer, "Min: %u Max: %u", minT, maxT);
        draw_string(10, y, buffer, 0xFFFF); y += 10;

        unsigned int avg = measCount > 0 ? totalT / measCount : 0;
        sprintf(buffer, "Avg: %u Total: %u", avg, totalT);
        draw_string(10, y, buffer, 0xFFFF); y += 10;

        TickType_t now = xTaskGetTickCount();
        unsigned int activeMs = (now - startTick) * portTICK_PERIOD_MS;
        sprintf(buffer, "Active: %u ms", activeMs);
        draw_string(10, y, buffer, 0xFFFF);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
