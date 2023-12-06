#include "globals.h"

volatile uint8_t error = 0;
// 1: RTC
// 2: GPS
// 4: Sensor access error
// 8: Sensor returned incoherent data
// 16: SD full
// 32: SD access error

Config config;