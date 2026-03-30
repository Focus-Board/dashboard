#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>

#define BATTERY_ADC_PIN 4       
#define BATTERY_MAX_VOLTAGE 4.2 
#define BATTERY_MIN_VOLTAGE 3.3 

// Initialize battery monitoring
void battery_init();

// Get battery voltage in volts
float battery_get_voltage();

// Get battery percentage (0-100)
int battery_get_percentage();

// Get battery status as string
const char* battery_get_status_string();

#endif
