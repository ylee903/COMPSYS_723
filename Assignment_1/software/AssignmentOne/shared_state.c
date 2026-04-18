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
