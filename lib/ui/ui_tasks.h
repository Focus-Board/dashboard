#ifndef UI_TASKS_H
#define UI_TASKS_H

#include <lvgl.h>
#include <Arduino.h>

// Tasks screen initialization
void ui_tasks_init();

// Show tasks screen
void ui_tasks_show();

// Update tasks with new data (JSON string)
void ui_tasks_update_tasks(String jsonData);

// Update header (time, WiFi, battery)
void ui_tasks_update_header(bool wifiConnected);

#endif
