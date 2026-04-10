#ifndef UI_HEADER_H
#define UI_HEADER_H

#include <lvgl.h>
#include <Arduino.h>

typedef struct {
  lv_obj_t *container;
  lv_obj_t *time_label;
  lv_obj_t *left_label;
  lv_obj_t *right_label;
} ui_header_t;

void ui_header_create(lv_obj_t *parent, ui_header_t *header);
void ui_header_update_time(ui_header_t *header);
void ui_header_set_left_text(ui_header_t *header, const char *text);
void ui_header_set_right_text(ui_header_t *header, const char *text);

#endif
