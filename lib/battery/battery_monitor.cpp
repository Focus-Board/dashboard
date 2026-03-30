#include "battery_monitor.h"

#define ADC_SAMPLES 10
#define ADC_MAX_VALUE 4095
#define ADC_REFERENCE_VOLTAGE 3.3

void battery_init() {
  pinMode(BATTERY_ADC_PIN, INPUT);
  analogReadResolution(12); // 12-bit ADC resolution
  
  // Take a few dummy reads to stabilize ADC
  for (int i = 0; i < 5; i++) {
    analogRead(BATTERY_ADC_PIN);
    delay(10);
  }
}

float battery_get_voltage() {
  // Average multiple readings for stability
  long sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(BATTERY_ADC_PIN);
    delay(5);
  }
  
  int avg_reading = sum / ADC_SAMPLES;
  
  // Convert ADC reading to voltage
  // Note: You may need to adjust the multiplier based on your voltage divider
  float voltage = (avg_reading * ADC_REFERENCE_VOLTAGE) / ADC_MAX_VALUE;
  
  // If using a voltage divider (common on battery monitor circuits)
  // multiply by the divider ratio. Example for 1:2 divider:
  voltage *= 2.0;
  
  return voltage;
}

int battery_get_percentage() {
  float voltage = battery_get_voltage();
  
  // Clamp voltage to valid range
  if (voltage > BATTERY_MAX_VOLTAGE) {
    voltage = BATTERY_MAX_VOLTAGE;
  }
  if (voltage < BATTERY_MIN_VOLTAGE) {
    voltage = BATTERY_MIN_VOLTAGE;
  }
  
  // Linear interpolation between min and max voltage
  float percentage = ((voltage - BATTERY_MIN_VOLTAGE) / 
                      (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0;
  
  return (int)percentage;
}

const char* battery_get_status_string() {
  static char status[16];
  int percentage = battery_get_percentage();
  
  if (percentage > 80) {
    snprintf(status, sizeof(status), "Batt: %d%%", percentage);
  } else if (percentage > 20) {
    snprintf(status, sizeof(status), "Batt: %d%%", percentage);
  } else {
    snprintf(status, sizeof(status), "Low: %d%%", percentage);
  }
  
  return status;
}
