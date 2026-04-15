#include "ui_calendar.h"
#include "ui_header.h"
#include <ArduinoJson.h>
#include <time.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define HEADER_HEIGHT 30
#define TIME_LABEL_WIDTH 60
#define DAY_LABEL_HEIGHT 30
#define ALL_DAY_HEIGHT 35
#define CALENDAR_START_HOUR 7
#define CALENDAR_END_HOUR 23
#define CALENDAR_HOURS (CALENDAR_END_HOUR - CALENDAR_START_HOUR)
#define LINE_THICKNESS 1      
#define CURRENT_DAY_LINE_THICKNESS 3
#define EVENT_PADDING 2       
#define MIN_EVENT_HEIGHT 20   
#define TIMEZONE_OFFSET -4 // EST   


static lv_obj_t *screen_calendar = NULL;
static ui_header_t header;
static lv_obj_t *calendar_container = NULL;
static lv_obj_t *day_labels[7];
static lv_obj_t *time_labels[CALENDAR_HOURS + 1];
static lv_obj_t *all_day_containers[7];
static lv_obj_t *grid_lines[50]; 
static int grid_line_count = 0;
static lv_obj_t *event_blocks[50]; 
static int event_block_count = 0;

#define MAX_EVENTS 50
struct CalendarEvent {
  String name;
  time_t start_time;
  time_t end_time;
  bool is_all_day;
  int day_of_week;
};

static CalendarEvent events[MAX_EVENTS];
static int event_count = 0;

static int get_day_of_week_from_monday(struct tm *timeinfo);
static int time_to_pixel_y(int hour, int minute);
static void draw_event_block(CalendarEvent &event, int col, int overlap_count, int overlap_index);
static void draw_time_grid();
static void draw_current_time_indicator();

void ui_calendar_init() {
  screen_calendar = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen_calendar, lv_color_white(), 0);
  
  ui_header_create(screen_calendar, &header);
  
  calendar_container = lv_obj_create(screen_calendar);
  lv_obj_set_size(calendar_container, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT);
  lv_obj_set_pos(calendar_container, 0, HEADER_HEIGHT);
  lv_obj_set_style_bg_color(calendar_container, lv_color_white(), 0);
  lv_obj_set_style_border_width(calendar_container, 0, 0);
  lv_obj_set_style_pad_all(calendar_container, 0, 0);
  
  int col_width = (SCREEN_WIDTH - TIME_LABEL_WIDTH) / 7;
  
  const char* day_names[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  for (int i = 0; i < 7; i++) {
    day_labels[i] = lv_label_create(calendar_container);
    lv_obj_set_pos(day_labels[i], TIME_LABEL_WIDTH + i * col_width + col_width/2 - 20, 5);
    lv_label_set_text(day_labels[i], day_names[i]);
    lv_obj_set_style_text_font(day_labels[i], &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(day_labels[i], lv_color_black(), 0);
  }
  
  for (int i = 0; i < 7; i++) {
    all_day_containers[i] = lv_obj_create(calendar_container);
    lv_obj_set_size(all_day_containers[i], col_width - 4, ALL_DAY_HEIGHT);
    lv_obj_set_pos(all_day_containers[i], TIME_LABEL_WIDTH + i * col_width + 2, DAY_LABEL_HEIGHT);
    lv_obj_set_style_bg_color(all_day_containers[i], lv_color_white(), 0);
    lv_obj_set_style_border_width(all_day_containers[i], LINE_THICKNESS, 0);
    lv_obj_set_style_border_color(all_day_containers[i], lv_color_black(), 0);
    lv_obj_set_style_pad_all(all_day_containers[i], EVENT_PADDING, 0);
    lv_obj_set_style_radius(all_day_containers[i], 0, 0);
  }
  
  int time_grid_height = SCREEN_HEIGHT - HEADER_HEIGHT - DAY_LABEL_HEIGHT - ALL_DAY_HEIGHT - 5;
  for (int i = 0; i <= CALENDAR_HOURS; i++) {
    time_labels[i] = lv_label_create(calendar_container);
    int hour = CALENDAR_START_HOUR + i;
    int y = DAY_LABEL_HEIGHT + ALL_DAY_HEIGHT + (i * time_grid_height) / CALENDAR_HOURS;
    lv_obj_set_pos(time_labels[i], 5, y - 8);
    
    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:00", hour);
    lv_label_set_text(time_labels[i], time_str);
    lv_obj_set_style_text_font(time_labels[i], &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(time_labels[i], lv_color_black(), 0);
  }
  
  draw_time_grid();
}

void ui_calendar_show() {
  lv_screen_load(screen_calendar);
  ui_header_update_time(&header);
  
  for (int i = 0; i < event_block_count; i++) {
    lv_obj_del(event_blocks[i]);
  }
  event_block_count = 0;
  
  for (int i = 0; i < 7; i++) {
    lv_obj_clean(all_day_containers[i]);
  }
  
  struct tm now_tm;
  getLocalTime(&now_tm);
  time_t now = mktime(&now_tm);
  
  int current_dow = get_day_of_week_from_monday(&now_tm);
  
  time_t week_start = now - (current_dow * 86400);
  struct tm week_start_tm;
  localtime_r(&week_start, &week_start_tm);
  week_start_tm.tm_hour = 0;
  week_start_tm.tm_min = 0;
  week_start_tm.tm_sec = 0;
  week_start = mktime(&week_start_tm);
  
  time_t week_end = week_start + (7 * 86400);
  
  for (int i = 0; i < event_count; i++) {
    if (events[i].start_time < week_start || events[i].start_time >= week_end) {
      continue;
    }
    
    int overlap_count = 0;
    int overlap_index = 0;
    
    if (!events[i].is_all_day) {
      for (int j = 0; j < event_count; j++) {
        if (events[i].day_of_week == events[j].day_of_week && !events[j].is_all_day) {
          if (events[j].start_time >= week_start && events[j].start_time < week_end) {
            if (events[j].start_time < events[i].end_time && 
                events[j].end_time > events[i].start_time) {
              if (j < i) overlap_index++;
              overlap_count++;
            }
          }
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
      evt.start_time = mktime(&start_tm) + (TIMEZONE_OFFSET * 3600);
      evt.day_of_week = get_day_of_week_from_monday(&start_tm);
    }
    
    if (strlen(end_str) > 0) {
      strptime(end_str, "%Y-%m-%dT%H:%M:%S", &end_tm);
      evt.end_time = mktime(&end_tm) + (TIMEZONE_OFFSET * 3600);
    }
    
    event_count++;
  }
  
  Serial.print("Loaded ");
  Serial.print(event_count);
  Serial.println(" events");
}

void ui_calendar_update_header(bool wifiConnected) {
  ui_header_update_time(&header);
  ui_header_set_left_text(&header, "Focusboard: Calendar");
  ui_header_set_right_text(&header, "Batt: 100%");
}

static int get_day_of_week_from_monday(struct tm *timeinfo) {
  int dow = timeinfo->tm_wday;
  return (dow == 0) ? 6 : dow - 1;
}

static int time_to_pixel_y(int hour, int minute) {
  int time_grid_height = SCREEN_HEIGHT - HEADER_HEIGHT - DAY_LABEL_HEIGHT - ALL_DAY_HEIGHT - 5;
  int total_minutes = (hour - CALENDAR_START_HOUR) * 60 + minute;
  int calendar_minutes = CALENDAR_HOURS * 60;
  
  return DAY_LABEL_HEIGHT + ALL_DAY_HEIGHT + 
         (total_minutes * time_grid_height) / calendar_minutes;
}

static void draw_event_block(CalendarEvent &event, int col, int overlap_count, int overlap_index) {
  if (event.is_all_day) {
    lv_obj_t *all_day_label = lv_label_create(all_day_containers[col]);
    lv_label_set_text(all_day_label, event.name.c_str());
    lv_obj_set_style_text_font(all_day_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(all_day_label, lv_color_black(), 0);
    lv_obj_set_pos(all_day_label, EVENT_PADDING, EVENT_PADDING);
    lv_label_set_long_mode(all_day_label, LV_LABEL_LONG_WRAP);
    
    int col_width = (SCREEN_WIDTH - TIME_LABEL_WIDTH) / 7;
    lv_obj_set_width(all_day_label, col_width - (EVENT_PADDING * 4));
    return;
  }
  
  struct tm start_tm, end_tm;
  localtime_r(&event.start_time, &start_tm);
  localtime_r(&event.end_time, &end_tm);
  
  int start_y = time_to_pixel_y(start_tm.tm_hour, start_tm.tm_min);
  int end_y = time_to_pixel_y(end_tm.tm_hour, end_tm.tm_min);
  int height = end_y - start_y;
  
  if (height < MIN_EVENT_HEIGHT) height = MIN_EVENT_HEIGHT;
  
  int col_width = (SCREEN_WIDTH - TIME_LABEL_WIDTH) / 7;
  int block_width = col_width / (overlap_count > 0 ? overlap_count : 1);
  int x = TIME_LABEL_WIDTH + col * col_width + overlap_index * block_width;
  
  lv_obj_t *event_block = lv_obj_create(calendar_container);
  lv_obj_set_size(event_block, block_width - 4, height);
  lv_obj_set_pos(event_block, x + 2, start_y);
  lv_obj_set_style_bg_color(event_block, lv_color_hex(0x404040), 0);
  lv_obj_set_style_bg_opa(event_block, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(event_block, 0, 0);
  lv_obj_set_style_pad_all(event_block, EVENT_PADDING, 0);
  lv_obj_set_style_radius(event_block, 0, 0);
  
  lv_obj_t *event_label = lv_label_create(event_block);
  lv_label_set_text(event_label, event.name.c_str());
  lv_obj_set_style_text_font(event_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(event_label, lv_color_white(), 0);
  lv_obj_set_pos(event_label, EVENT_PADDING, EVENT_PADDING);
  lv_label_set_long_mode(event_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(event_label, block_width - (EVENT_PADDING * 4));
  
  event_blocks[event_block_count++] = event_block;
}

static void draw_time_grid() {
  int time_grid_height = SCREEN_HEIGHT - HEADER_HEIGHT - DAY_LABEL_HEIGHT - ALL_DAY_HEIGHT - 5;
  int col_width = (SCREEN_WIDTH - TIME_LABEL_WIDTH) / 7;
  
  grid_line_count = 0;

  struct tm timeinfo;
  int current_day = -1;
  if (getLocalTime(&timeinfo)) {
    current_day = get_day_of_week_from_monday(&timeinfo);
  }

  for (int i = 0; i <= CALENDAR_HOURS; i++) {
    lv_obj_t *line_obj = lv_obj_create(calendar_container);
    int y = DAY_LABEL_HEIGHT + ALL_DAY_HEIGHT + (i * time_grid_height) / CALENDAR_HOURS;
    lv_obj_set_size(line_obj, TIME_LABEL_WIDTH + 7 * col_width, LINE_THICKNESS);
    lv_obj_set_pos(line_obj, TIME_LABEL_WIDTH, y);
    lv_obj_set_style_bg_color(line_obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(line_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line_obj, 0, 0);
    grid_lines[grid_line_count++] = line_obj;
  }
  
  for (int i = 0; i <= 7; i++) {
    lv_obj_t *line_obj = lv_obj_create(calendar_container);
    int x = TIME_LABEL_WIDTH + i * col_width;
    
    bool is_current_day_line = (i == current_day || i == current_day + 1);
    int thickness = is_current_day_line ? CURRENT_DAY_LINE_THICKNESS : LINE_THICKNESS;
    
    lv_obj_set_size(line_obj, thickness, time_grid_height);
    lv_obj_set_pos(line_obj, x, DAY_LABEL_HEIGHT + ALL_DAY_HEIGHT);
    lv_obj_set_style_bg_color(line_obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(line_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line_obj, 0, 0);
    grid_lines[grid_line_count++] = line_obj;
  }
}

static void draw_current_time_indicator() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  
  int current_y = time_to_pixel_y(timeinfo.tm_hour, timeinfo.tm_min);
  
  lv_obj_t *time_line = lv_obj_create(calendar_container);
  lv_obj_set_size(time_line, 800, LINE_THICKNESS * 4); 
  lv_obj_set_pos(time_line, 60, current_y - LINE_THICKNESS);
  lv_obj_set_style_bg_color(time_line, lv_color_black(), 0); 
  lv_obj_set_style_bg_opa(time_line, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(time_line, 1, 0);
  lv_obj_set_style_border_color(time_line, lv_color_white(), 0);
}