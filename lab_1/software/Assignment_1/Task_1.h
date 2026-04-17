#ifndef TASK_1_H_
#define TASK_1_H_

#include "Main.h"

/* Based on conceptual design naming */
void FrequencyRelayISR(void *context, alt_u32 id);
void FrequencyTask(void *pvParameters);
void ROCOFTask(void *pvParameters);

#endif
