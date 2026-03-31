#include "ui_calendar.h"
#include <ArduinoJson.h>
#include <time.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

// Calendar layout
#define HEADER_HEIGHT 40
#define TIME_LABEL_WIDTH 50
#define DAY_LABEL_HEIGHT 30
#define ALL_DAY_HEIGHT 40
#define CALENDAR_START_HOUR 7
#define CALENDAR_END_HOUR 23
#define CALENDAR_HOURS (CALENDAR_END_HOUR - CALENDAR_START_HOUR)
#define MINUTES_PER_STEP 15

static lv_obj_t *screen_calendar = NULL;
static lv_obj_t *header_container = NULL;
static lv_obj_t *time_label = NULL;
static lv_obj_t *wifi_label = NULL;
static lv_obj_t *battery_label = NULL;
static lv_obj_t *calendar_container = NULL;
static lv_obj_t *day_labels[7];
static lv_obj_t *time_labels[CALENDAR_HOURS + 1];
static lv_obj_t *all_day_containers[7];

#define MAX_EVENTS 50
struct CalendarEvent {
  String name;
  time_t start_time;
  time_t end_time;
  bool is_all_day;
  int day_of_week; // 0 = Monday
};

static CalendarEvent events[MAX_EVENTS];
static int event_count = 0;

static int get_day_of_week_from_monday(struct tm *timeinfo);
static int time_to_pixel_y(int hour, int minute);
static void draw_event_block(CalendarEvent &event, int col, int overlap_count, int overlap_index);
static void clear_calendar_events();
static void draw_time_grid();
static void draw_current_time_indicator();

void ui_calendar_init() {
  screen_calendar = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen_calendar, lv_color_white(), 0);
  
  header_container = lv_obj_create(screen_calendar);
  lv_obj_set_size(header_container, SCREEN_WIDTH, HEADER_HEIGHT);
  lv_obj_set_pos(header_container, 0, 0);
  lv_obj_set_style_bg_color(header_container, lv_color_white(), 0);
  lv_obj_set_style_border_width(header_container, 1, 0);
  lv_obj_set_style_pad_all(header_container, 5, 0);
  
  time_label = lv_label_create(header_container);
  lv_obj_set_pos(time_label, 5, 5);
  lv_label_set_text(time_label, "00:00");
  
  wifi_label = lv_label_create(header_container);
  lv_obj_set_pos(wifi_label, 300, 5);
  lv_label_set_text(wifi_label, "WiFi: --");
  
  battery_label = lv_label_create(header_container);
  lv_obj_set_pos(battery_label, 600, 5);
  lv_label_set_text(battery_label, "Batt: --");
  
  calendar_container = lv_obj_create(screen_calendar);
  lv_obj_set_size(calendar_container, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT);
  lv_obj_set_pos(calendar_container, 0, HEADER_HEIGHT);
  lv_obj_set_style_bg_color(calendar_container, lv_color_white(), 0);
  lv_obj_set_style_pad_all(calendar_container, 0, 0);
  
  int col_width = (SCREEN_WIDTH - TIME_LABEL_WIDTH) / 7;
  
  const char* day_names[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  for (int i = 0; i < 7; i++) {
    day_labels[i] = lv_label_create(calendar_container);
    lv_obj_set_pos(day_labels[i], TIME_LABEL_WIDTH + i * col_width + col_width/2 - 15, 5);
    lv_label_set_text(day_labels[i], day_names[i]);
    lv_obj_set_style_text_font(day_labels[i], &lv_font_montserrat_14, 0);
  }
  
  // all day events
  for (int i = 0; i < 7; i++) {
    all_day_containers[i] = lv_obj_create(calendar_container);
    lv_obj_set_size(all_day_containers[i], col_width - 2, ALL_DAY_HEIGHT);
    lv_obj_set_pos(all_day_containers[i], TIME_LABEL_WIDTH + i * col_width, 
                   DAY_LABEL_HEIGHT);
    lv_obj_set_style_bg_color(all_day_containers[i], lv_color_white(), 0);
    lv_obj_set_style_border_width(all_day_containers[i], 1, 0);
  }
  
  // hour labels
  int time_grid_height = SCREEN_HEIGHT - HEADER_HEIGHT - DAY_LABEL_HEIGHT - ALL_DAY_HEIGHT;
  for (int i = 0; i <= CALENDAR_HOURS; i++) {
    time_labels[i] = lv_label_create(calendar_container);
    int hour = CALENDAR_START_HOUR + i;
    int y = DAY_LABEL_HEIGHT + ALL_DAY_HEIGHT + (i * time_grid_height) / CALENDAR_HOURS;
    lv_obj_set_pos(time_labels[i], 5, y - 8);
    
    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:00", hour);
    lv_label_set_text(time_labels[i], time_str);
    lv_obj_set_style_text_font(time_labels[i], &lv_font_montserrat_12, 0);
  }
  
  draw_time_grid();
}

void ui_calendar_show() {
  lv_screen_load(screen_calendar);
  
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char time_str[6];
    strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
    lv_label_set_text(time_label, time_str);
  }
  
  clear_calendar_events();

  for (int i = 0; i < event_count; i++) {
    int overlap_count = 0;
    int overlap_index = 0;
    
    for (int j = 0; j < event_count; j++) {
      if (events[i].day_of_week == events[j].day_of_week && !events[j].is_all_day) {
        if (events[j].start_time < events[i].end_time && 
            events[j].end_time > events[i].start_time) {
          if (j < i) overlap_index++;
          overlap_count++;
        }
      }
    }
    
    draw_event_block(events[i], events[i].day_of_week, overlap_count, overlap_index);
  }
  
  draw_current_time_indicator();
}

void ui_calendar_update_events(String jsonData) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }
  
  event_count = 0;
  JsonArray eventsArray = doc.as<JsonArray>();
  
  for (JsonObject eventObj : eventsArray) {
    if (event_count >= MAX_EVENTS) break;
    
    CalendarEvent &evt = events[event_count];
    
    evt.name = eventObj["title"] | "Untitled";
    evt.is_all_day = eventObj["all_day"] | false;
    
    const char* start_str = eventObj["start_time"] | "";
    const char* end_str = eventObj["end_time"] | "";
    
    struct tm start_tm = {0}, end_tm = {0};
    if (strlen(start_str) > 0) {
      strptime(start_str, "%Y-%m-%dT%H:%M:%S", &start_tm);
      evt.start_time = mktime(&start_tm);
      evt.day_of_week = get_day_of_week_from_monday(&start_tm);
    }
    
    if (strlen(end_str) > 0) {
      strptime(end_str, "%Y-%m-%dT%H:%M:%S", &end_tm);
      evt.end_time = mktime(&end_tm);
    }
    
    event_count++;
  }
  
  Serial.print("Loaded ");
  Serial.print(event_count);
  Serial.println(" events");
}

void ui_calendar_update_header(bool wifiConnected) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char time_str[6];
    strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
    lv_label_set_text(time_label, time_str);
  }
  
  lv_label_set_text(wifi_label, wifiConnected ? "WiFi: OK" : "WiFi: --");
  
  // TODO: read the actual voltage
  lv_label_set_text(battery_label, "Batt: OK");
}

static int get_day_of_week_from_monday(struct tm *timeinfo) {
  // Convert Sunday=0 to Monday=0 format
  int dow = timeinfo->tm_wday;
  return (dow == 0) ? 6 : dow - 1;
}

static int time_to_pixel_y(int hour, int minute) {
  int time_grid_height = SCREEN_HEIGHT - HEADER_HEIGHT - DAY_LABEL_HEIGHT - ALL_DAY_HEIGHT;
  int total_minutes = (hour - CALENDAR_START_HOUR) * 60 + minute;
  int calendar_minutes = CALENDAR_HOURS * 60;
  
  return DAY_LABEL_HEIGHT + ALL_DAY_HEIGHT + 
         (total_minutes * time_grid_height) / calendar_minutes;
}

static void draw_event_block(CalendarEvent &event, int col, int overlap_count, int overlap_index) {

  if (event.is_all_day) {
    lv_obj_t *all_day_label = lv_label_create(all_day_containers[col]);
    lv_label_set_text(all_day_label, event.name.c_str());
    lv_obj_set_style_text_font(all_day_label, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(all_day_label, 2, 2);
    return;
  }
  
  struct tm start_tm, end_tm;
  localtime_r(&event.start_time, &start_tm);
  localtime_r(&event.end_time, &end_tm);
  
  int start_y = time_to_pixel_y(start_tm.tm_hour, start_tm.tm_min);
  int end_y = time_to_pixel_y(end_tm.tm_hour, end_tm.tm_min);
  int height = end_y - start_y;
  
  if (height < 10) height = 10; // Minimum height
  
  // Calculate position and width based on overlaps
  int col_width = (SCREEN_WIDTH - TIME_LABEL_WIDTH) / 7;
  int block_width = col_width / overlap_count;
  int x = TIME_LABEL_WIDTH + col * col_width + overlap_index * block_width;
  
  // event bloc
  lv_obj_t *event_block = lv_obj_create(calendar_container);
  lv_obj_set_size(event_block, block_width - 2, height);
  lv_obj_set_pos(event_block, x, start_y);
  lv_obj_set_style_bg_color(event_block, lv_color_hex(0xE0E0E0), 0);
  lv_obj_set_style_border_width(event_block, 1, 0);
  lv_obj_set_style_border_color(event_block, lv_color_black(), 0);
  
  // label
  lv_obj_t *event_label = lv_label_create(event_block);
  lv_label_set_text(event_label, event.name.c_str());
  lv_obj_set_style_text_font(event_label, &lv_font_montserrat_10, 0);
  lv_obj_set_pos(event_label, 2, 2);
  lv_label_set_long_mode(event_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(event_label, block_width - 6);
}

static void clear_calendar_events() {
  lv_obj_clean(calendar_container);
}

static void draw_time_grid() {
  // horizontal lines
  int time_grid_height = SCREEN_HEIGHT - HEADER_HEIGHT - DAY_LABEL_HEIGHT - ALL_DAY_HEIGHT;
  int col_width = (SCREEN_WIDTH - TIME_LABEL_WIDTH) / 7;
  
  for (int i = 0; i <= CALENDAR_HOURS; i++) {
    lv_obj_t *line_obj = lv_obj_create(calendar_container);
    int y = DAY_LABEL_HEIGHT + ALL_DAY_HEIGHT + (i * time_grid_height) / CALENDAR_HOURS;
    lv_obj_set_size(line_obj, SCREEN_WIDTH - TIME_LABEL_WIDTH, 1);
    lv_obj_set_pos(line_obj, TIME_LABEL_WIDTH, y);
    lv_obj_set_style_bg_color(line_obj, lv_color_hex(0xCCCCCC), 0);
  }
  
  // vertical lines
  for (int i = 0; i <= 7; i++) {
    lv_obj_t *line_obj = lv_obj_create(calendar_container);
    int x = TIME_LABEL_WIDTH + i * col_width;
    lv_obj_set_size(line_obj, 1, time_grid_height);
    lv_obj_set_pos(line_obj, x, DAY_LABEL_HEIGHT + ALL_DAY_HEIGHT);
    lv_obj_set_style_bg_color(line_obj, lv_color_hex(0xCCCCCC), 0);
  }
}

static void draw_current_time_indicator() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  
  int current_dow = get_day_of_week_from_monday(&timeinfo);
  int current_y = time_to_pixel_y(timeinfo.tm_hour, timeinfo.tm_min);
  
  // current time indicator
  lv_obj_t *time_line = lv_obj_create(calendar_container);
  lv_obj_set_size(time_line, 2, 10);
  lv_obj_set_pos(time_line, TIME_LABEL_WIDTH - 10, current_y - 5);
  lv_obj_set_style_bg_color(time_line, lv_color_hex(0xFF0000), 0);
}
