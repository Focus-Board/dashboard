#ifndef UI_CALENDAR_H
#define UI_CALENDAR_H

#include <lvgl.h>
#include <Arduino.h>

// Calendar screen initialization
void ui_calendar_init();

// Show calendar screen
void ui_calendar_show();

// Update calendar with new events data (JSON string)
void ui_calendar_update_events(String jsonData);

// Update header (time, WiFi, battery)
void ui_calendar_update_header(bool wifiConnected);

#endif
