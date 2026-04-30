#ifndef ISR_HANDLERS_H
#define ISR_HANDLERS_H

#include "alt_types.h"

void FrequencyAnalyserISR(void *context);
void PushButtonISR(void *context);
void KeyboardISR(void *context, alt_u32 id);

#endif


// Explanation of the above (with good formatting, i.e. multi line comments, indentation, etc.):
/*
This header file declares the prototypes for three interrupt service routines (ISRs) that will be defined   
in the corresponding isr_handlers.c file. These ISRs are responsible for handling interrupts from the frequency analyser, push buttons, and keyboard. The functions take a context pointer (and an additional id for the keyboard ISR) as parameters, which will be used to access hardware registers and communicate with FreeRTOS tasks. By including this header file in other source files, we can ensure that the ISRs are properly declared and can be linked correctly during compilation.
*/  