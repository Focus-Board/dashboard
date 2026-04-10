#ifndef UI_JOURNAL_H
#define UI_JOURNAL_H

#include <lvgl.h>
#include <Arduino.h>

void ui_journal_init();
void ui_journal_show();
void ui_journal_update_entries(String jsonData);
void ui_journal_update_header(bool wifiConnected);

#endif
