#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/semphr.h"

extern SemaphoreHandle_t sharedStateMutex;

extern unsigned int userRequestedLoads;   // raw switch requests
extern unsigned int allowedLoads;         // filtered requests during management
extern unsigned int relayShedLoads;       // loads currently shed by relay
extern unsigned int actualLoads;          // final outputs
extern unsigned int maintenanceMode;      // 0 = normal, 1 = maintenance

extern unsigned int systemUnstable;       // 0 = stable, 1 = unstable
extern unsigned int managingLoads;        // 0 = normal, 1 = currently managing

extern float currentFrequencyHz;
extern float currentROCOF;
extern float frequencyThreshold;
extern float rocofThreshold;

extern TickType_t detectionTick;
extern TickType_t firstShedTick;
extern unsigned int timingArmed;
extern unsigned int firstShedDone;

extern unsigned int recentTimes[5];
extern unsigned int recentIndex;
extern unsigned int recentCount;

extern unsigned int minTime;
extern unsigned int maxTime;
extern unsigned int totalTime;
extern unsigned int measurementCount;

extern TickType_t systemStartTick;

#endif
