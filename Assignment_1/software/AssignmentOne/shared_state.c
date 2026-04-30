#include "shared_state.h"

SemaphoreHandle_t sharedStateMutex = NULL;

unsigned int userRequestedLoads = 0;
unsigned int allowedLoads = 0;
unsigned int relayShedLoads = 0;
unsigned int actualLoads = 0;
unsigned int maintenanceMode = 0;

unsigned int systemUnstable = 0;
unsigned int managingLoads = 0;

float currentFrequencyHz = 50.0f;
float currentROCOF = 0.0f;
float frequencyThreshold = 48.5f;
float rocofThreshold = 20.0f;

TickType_t lastAnalyserIsrTick = 0;
TickType_t detectionTick = 0;
TickType_t firstShedTick = 0;
unsigned int timingArmed = 0;
unsigned int firstShedDone = 0;

unsigned int recentTimes[RECENT_TIMES_COUNT] = {0};
unsigned int recentIndex = 0;
unsigned int recentCount = 0;

unsigned int minTime = 0xFFFFFFFF;
unsigned int maxTime = 0;
unsigned int totalTime = 0;
unsigned int measurementCount = 0;

TickType_t systemStartTick = 0;

// Explanation of the above (with good formatting, i.e. multi line comments, indentation, etc.):
/************************************************************
 * This file defines the shared state variables and a mutex to protect access to them. The shared state includes:
 * - userRequestedLoads: A bitmask representing which loads the user has requested to be on.
 * - allowedLoads: A bitmask representing which loads are currently allowed to be on based on user requests and system decisions.
 * - relayShedLoads: A bitmask representing which loads are currently shed (turned off) by the relay.
 * - actualLoads: A bitmask representing which loads are actually on, considering both user requests and shedding.
 * - maintenanceMode: A flag indicating whether the system is in maintenance mode, where all requested loads are allowed regardless of frequency or ROCOF.
 * - systemUnstable: A flag indicating whether the system is currently considered unstable based on frequency and ROCOF measurements.
 * - managingLoads: A flag indicating whether the system is currently managing load shedding (i.e., has shed any loads).
 * - currentFrequencyHz: The most recent frequency measurement in Hz.
 * - currentROCOF: The most recent ROCOF measurement.
 * - frequencyThreshold: The frequency threshold below which the system is considered unstable.
 * - rocofThreshold: The ROCOF threshold above which the system is considered unstable.
 * - lastAnalyserIsrTick: The tick count at which the last frequency analyser ISR was called.
 * - detectionTick: The tick count at which instability was detected.
 * - firstShedTick: The tick count at which the first load was shed due to instability.
 * - timingArmed: A flag indicating whether timing for load shedding has been armed (i.e., instability has been detected but no loads have been shed yet).
 * - firstShedDone: A flag indicating whether the first load shedding actuation has been done since instability was detected.
 * - recentTimes, recentIndex, recentCount, minTime, maxTime, totalTime, measurementCount: Variables used for tracking timing statistics of load shedding actuation delays.
 * - systemStartTick: The tick count at which the system started, used for calculating uptime or other time-based metrics.
 ************************************************************/
 