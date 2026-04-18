#include <stdio.h>
#include <string.h>

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/semphr.h"
#include "shared_state.h"

#include "system.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"

#define GRAPH_WIDTH   60
#define FREQ_GRAPH_TOP  8
#define FREQ_GRAPH_HEIGHT 10
#define ROCOF_GRAPH_TOP  22
#define ROCOF_GRAPH_HEIGHT 10

static float freqHistory[GRAPH_WIDTH];
static float rocofHistory[GRAPH_WIDTH];
static int historyIndex = 0;
static int historyFilled = 0;

static alt_up_char_buffer_dev *char_buffer = NULL;

static void clear_text_screen(void)
{
    if (char_buffer != NULL)
    {
        alt_up_char_buffer_clear(char_buffer);
    }
}

static void draw_text(int x, int y, const char *text)
{
    if (char_buffer != NULL)
    {
        alt_up_char_buffer_string(char_buffer, text, x, y);
    }
}

static void draw_char(int x, int y, char c)
{
    char s[2];
    s[0] = c;
    s[1] = '\0';
    draw_text(x, y, s);
}

static int map_frequency_to_y(float f)
{
    float minF = 45.0f;
    float maxF = 55.0f;

    if (f < minF) f = minF;
    if (f > maxF) f = maxF;

    return FREQ_GRAPH_TOP +
           (int)((maxF - f) * (FREQ_GRAPH_HEIGHT - 1) / (maxF - minF));
}

static int map_rocof_to_y(float r)
{
    float minR = -30.0f;
    float maxR = 30.0f;

    if (r < minR) r = minR;
    if (r > maxR) r = maxR;

    return ROCOF_GRAPH_TOP +
           (int)((maxR - r) * (ROCOF_GRAPH_HEIGHT - 1) / (maxR - minR));
}

static void draw_graph_axes(void)
{
    int i;

    /* Frequency graph border */
    draw_text(0, FREQ_GRAPH_TOP - 1, "Frequency Graph (45-55 Hz)");
    for (i = 0; i < GRAPH_WIDTH; i++)
    {
        draw_char(i + 1, FREQ_GRAPH_TOP + FREQ_GRAPH_HEIGHT, '-');
    }
    for (i = 0; i < FREQ_GRAPH_HEIGHT; i++)
    {
        draw_char(0, FREQ_GRAPH_TOP + i, '|');
    }

    /* ROCOF graph border */
    draw_text(0, ROCOF_GRAPH_TOP - 1, "ROCOF Graph (-30 to 30 Hz/s)");
    for (i = 0; i < GRAPH_WIDTH; i++)
    {
        draw_char(i + 1, ROCOF_GRAPH_TOP + ROCOF_GRAPH_HEIGHT, '-');
    }
    for (i = 0; i < ROCOF_GRAPH_HEIGHT; i++)
    {
        draw_char(0, ROCOF_GRAPH_TOP + i, '|');
    }
}

static void draw_history_graphs(void)
{
    int i;
    int count;
    int idx;
    int x;
    int y;

    count = historyFilled ? GRAPH_WIDTH : historyIndex;

    /* Draw frequency history as '*' */
    for (i = 0; i < count; i++)
    {
        idx = (historyFilled ? historyIndex : 0) + i;
        idx %= GRAPH_WIDTH;

        x = i + 1;
        y = map_frequency_to_y(freqHistory[idx]);
        draw_char(x, y, '*');
    }

    /* Draw ROCOF history as '+' */
    for (i = 0; i < count; i++)
    {
        idx = (historyFilled ? historyIndex : 0) + i;
        idx %= GRAPH_WIDTH;

        x = i + 1;
        y = map_rocof_to_y(rocofHistory[idx]);
        draw_char(x, y, '+');
    }
}

static void draw_load_states(unsigned int actual, unsigned int shed)
{
    char line1[80];
    char line2[80];
    int i;
    int pos = 0;

    strcpy(line1, "Loads ON : ");
    strcpy(line2, "Loads Shed: ");
    pos = (int)strlen(line1);

    for (i = 0; i < 5; i++)
    {
        line1[pos++] = (actual & (1U << i)) ? '1' : '0';
        line1[pos++] = ' ';
    }
    line1[pos] = '\0';

    pos = (int)strlen(line2);
    for (i = 0; i < 5; i++)
    {
        line2[pos++] = (shed & (1U << i)) ? '1' : '0';
        line2[pos++] = ' ';
    }
    line2[pos] = '\0';

    draw_text(0, 5, line1);
    draw_text(0, 6, line2);
}

void VGATask(void *pvParameters)
{
    float freq;
    float rocof;
    float freqThresh;
    float rocofThresh;
    unsigned int actual;
    unsigned int shed;
    unsigned int maintenance;
    unsigned int unstable;
    char line[80];

    char_buffer = alt_up_char_buffer_open_dev("/dev/video_character_buffer_with_dma_avalon_char_buffer_slave");

    while (1)
    {
        if (xSemaphoreTake(sharedStateMutex, portMAX_DELAY) == pdTRUE)
        {
            freq = currentFrequencyHz;
            rocof = currentROCOF;
            freqThresh = frequencyThreshold;
            rocofThresh = rocofThreshold;

            actual = actualLoads;
            shed = relayShedLoads;
            maintenance = maintenanceMode;
            unstable = systemUnstable;

            xSemaphoreGive(sharedStateMutex);
        }

        freqHistory[historyIndex] = freq;
        rocofHistory[historyIndex] = rocof;
        historyIndex++;

        if (historyIndex >= GRAPH_WIDTH)
        {
            historyIndex = 0;
            historyFilled = 1;
        }

        clear_text_screen();

        draw_text(0, 0, "LCFR STATUS");

        snprintf(line, sizeof(line), "Freq: %.2f Hz", freq);
        draw_text(0, 1, line);

        snprintf(line, sizeof(line), "ROCOF: %.2f Hz/s", rocof);
        draw_text(0, 2, line);

        snprintf(line, sizeof(line), "Freq Threshold: %.2f Hz", freqThresh);
        draw_text(0, 3, line);

        snprintf(line, sizeof(line), "ROCOF Threshold: %.2f Hz/s", rocofThresh);
        draw_text(0, 4, line);

        if (maintenance)
        {
            draw_text(35, 1, "MODE: MAINTENANCE");
        }
        else if (unstable)
        {
            draw_text(35, 1, "MODE: MANAGING");
        }
        else
        {
            draw_text(35, 1, "MODE: STABLE");
        }

        draw_load_states(actual, shed);
        draw_graph_axes();
        draw_history_graphs();

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
