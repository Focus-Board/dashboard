#include "ui_header.h"
#include <time.h>

#define SCREEN_WIDTH 800
#define HEADER_HEIGHT 30

void ui_header_create(lv_obj_t *parent, ui_header_t *header) {
  header->container = lv_obj_create(parent);
  lv_obj_set_size(header->container, SCREEN_WIDTH, HEADER_HEIGHT);
  lv_obj_set_pos(header->container, 0, 0);
  lv_obj_set_style_bg_color(header->container, lv_color_white(), 0);
  lv_obj_set_style_border_width(header->container, 0, 0);
  lv_obj_set_style_pad_all(header->container, 0, 0);
  
  lv_obj_t *bottom_line = lv_obj_create(header->container);
  lv_obj_set_size(bottom_line, SCREEN_WIDTH, 2);
  lv_obj_set_pos(bottom_line, 0, HEADER_HEIGHT - 2);
  lv_obj_set_style_bg_color(bottom_line, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(bottom_line, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(bottom_line, 0, 0);
  
  header->time_label = lv_label_create(header->container);
  lv_obj_set_pos(header->time_label, 375, 4);
  lv_label_set_text(header->time_label, "00:00");
  lv_obj_set_style_text_font(header->time_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(header->time_label, lv_color_black(), 0);
  
  header->left_label = lv_label_create(header->container);
  lv_obj_set_pos(header->left_label, 10, 6);
  lv_label_set_text(header->left_label, "");
  lv_obj_set_style_text_font(header->left_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(header->left_label, lv_color_black(), 0);
  
  header->right_label = lv_label_create(header->container);
  lv_obj_set_pos(header->right_label, 710, 6);
  lv_label_set_text(header->right_label, "");
  lv_obj_set_style_text_font(header->right_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(header->right_label, lv_color_black(), 0);
}

void ui_header_update_time(ui_header_t *header) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char time_str[6];
    strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
    lv_label_set_text(header->time_label, time_str);
  }
}

void ui_header_set_left_text(ui_header_t *header, const char *text) {
  lv_label_set_text(header->left_label, text);
}

void ui_header_set_right_text(ui_header_t *header, const char *text) {
  lv_label_set_text(header->right_label, text);
}
