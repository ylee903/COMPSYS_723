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
