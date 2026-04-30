#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/semphr.h"

#define RECENT_TIMES_COUNT 5

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

extern TickType_t lastAnalyserIsrTick;
extern TickType_t detectionTick;
extern TickType_t firstShedTick;
extern unsigned int timingArmed;
extern unsigned int firstShedDone;

extern unsigned int recentTimes[RECENT_TIMES_COUNT];
extern unsigned int recentIndex;
extern unsigned int recentCount;

extern unsigned int minTime;
extern unsigned int maxTime;
extern unsigned int totalTime;
extern unsigned int measurementCount;

extern TickType_t systemStartTick;

#endif

// Explanation of the above (with good formatting, i.e. multi line comments, indentation, etc.):
/*
This header file defines the shared state variables and a mutex to protect access to them. The shared state includes:
- userRequestedLoads: A bitmask representing which loads the user has requested to be on.
- allowedLoads: A bitmask representing which loads are currently allowed to be on based on user requests and system decisions.
- relayShedLoads: A bitmask representing which loads are currently shed (turned off) by the relay.
- actualLoads: A bitmask representing which loads are actually on, considering both user requests and                                                       
shedding.
- maintenanceMode: A flag indicating whether the system is in maintenance mode, where all requested loads are allowed regardless of frequency or ROCOF.
- systemUnstable: A flag indicating whether the system is currently considered unstable based on frequency and ROCOF measurements.
- managingLoads: A flag indicating whether the system is currently managing load shedding (i.e., has shed any loads).
- currentFrequencyHz: The most recent frequency measurement in Hz.
- currentROCOF: The most recent ROCOF measurement.
- frequencyThreshold: The frequency threshold below which the system is considered unstable.
- rocofThreshold: The ROCOF threshold above which the system is considered unstable.
- lastAnalyserIsrTick: The tick count at which the last frequency analyser ISR was called.
- detectionTick: The tick count at which instability was detected.
- firstShedTick: The tick count at which the first load was shed due to instability.
- timingArmed: A flag indicating whether timing for load shedding has been armed (i.e., instability has been detected but no loads have been shed yet).
- firstShedDone: A flag indicating whether the first load shedding actuation has been done since instability was detected.
- recentTimes, recentIndex, recentCount, minTime, maxTime, totalTime, measurementCount: Variables used for tracking timing statistics of load shedding actuation delays.
- systemStartTick: The tick count at which the system started, used for calculating uptime or other time-based metrics.
*/
