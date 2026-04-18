#ifndef ISR_HANDLERS_H
#define ISR_HANDLERS_H

#include "alt_types.h"

void FrequencyAnalyserISR(void *context);
void PushButtonISR(void *context);
void KeyboardISR(void *context, alt_u32 id);

#endif
