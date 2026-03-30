#include "ui_tasks.h"
#include <ArduinoJson.h>
#include <time.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define HEADER_HEIGHT 40
#define TASK_ITEM_HEIGHT 40
#define MAX_VISIBLE_TASKS 10

static lv_obj_t *screen_tasks = NULL;
static lv_obj_t *header_container = NULL;
static lv_obj_t *time_label = NULL;
static lv_obj_t *wifi_label = NULL;
static lv_obj_t *battery_label = NULL;
static lv_obj_t *tasks_container = NULL;
static lv_obj_t *task_list = NULL;

#define MAX_TASKS 50
struct Task {
  String name;
  bool completed;
  bool has_deadline;
  time_t deadline;
};

static Task tasks[MAX_TASKS];
static int task_count = 0;

void ui_tasks_init() {
  screen_tasks = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen_tasks, lv_color_white(), 0);
  
  header_container = lv_obj_create(screen_tasks);
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
  
  tasks_container = lv_obj_create(screen_tasks);
  lv_obj_set_size(tasks_container, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT);
  lv_obj_set_pos(tasks_container, 0, HEADER_HEIGHT);
  lv_obj_set_style_bg_color(tasks_container, lv_color_white(), 0);
  lv_obj_set_style_pad_all(tasks_container, 10, 0);
  
  task_list = lv_obj_create(tasks_container);
  lv_obj_set_size(task_list, SCREEN_WIDTH - 20, SCREEN_HEIGHT - HEADER_HEIGHT - 20);
  lv_obj_set_pos(task_list, 0, 0);
  lv_obj_set_style_bg_color(task_list, lv_color_white(), 0);
  lv_obj_set_style_border_width(task_list, 0, 0);
  lv_obj_set_flex_flow(task_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(task_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
}

void ui_tasks_show() {
  lv_screen_load(screen_tasks);
  
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char time_str[6];
    strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
    lv_label_set_text(time_label, time_str);
  }
  
  lv_obj_clean(task_list);
  
  for (int i = 0; i < task_count && i < MAX_VISIBLE_TASKS; i++) {
    // task container
    lv_obj_t *task_item = lv_obj_create(task_list);
    lv_obj_set_size(task_item, SCREEN_WIDTH - 40, TASK_ITEM_HEIGHT);
    lv_obj_set_style_bg_color(task_item, lv_color_white(), 0);
    lv_obj_set_style_border_width(task_item, 1, 0);
    lv_obj_set_style_border_color(task_item, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_pad_all(task_item, 5, 0);
    
    lv_obj_t *checkbox_visual = lv_obj_create(task_item);
    lv_obj_set_size(checkbox_visual, 20, 20);
    lv_obj_set_pos(checkbox_visual, 5, 5);
    lv_obj_set_style_border_width(checkbox_visual, 2, 0);
    lv_obj_set_style_border_color(checkbox_visual, lv_color_black(), 0);
    
    if (tasks[i].completed) {
      lv_obj_set_style_bg_color(checkbox_visual, lv_color_hex(0x000000), 0);
    } else {
      lv_obj_set_style_bg_color(checkbox_visual, lv_color_white(), 0);
    }
    
    // task label
    lv_obj_t *task_name = lv_label_create(task_item);
    lv_label_set_text(task_name, tasks[i].name.c_str());
    lv_obj_set_pos(task_name, 35, 5);
    lv_obj_set_style_text_font(task_name, &lv_font_montserrat_14, 0);
    
    if (tasks[i].completed) {
      lv_obj_set_style_text_color(task_name, lv_color_hex(0x888888), 0);
    }
    
    // Create deadline label if task has one
    if (tasks[i].has_deadline) {
      struct tm deadline_tm;
      localtime_r(&tasks[i].deadline, &deadline_tm);
      
      char deadline_str[32];
      strftime(deadline_str, sizeof(deadline_str), "Due: %b %d, %Y", &deadline_tm);
      
      lv_obj_t *deadline_label = lv_label_create(task_item);
      lv_label_set_text(deadline_label, deadline_str);
      lv_obj_set_pos(deadline_label, 450, 5);
      lv_obj_set_style_text_font(deadline_label, &lv_font_montserrat_12, 0);
      lv_obj_set_style_text_color(deadline_label, lv_color_hex(0x666666), 0);
    }
  }
}

void ui_tasks_update_tasks(String jsonData) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }
  
  task_count = 0;
  JsonArray tasksArray = doc.as<JsonArray>();
  
  for (JsonObject taskObj : tasksArray) {
    if (task_count >= MAX_TASKS) break;
    
    Task &task = tasks[task_count];
    
    // TODO: do the thing
    task.name = taskObj["name"] | taskObj["title"] | "Untitled Task";
    task.completed = taskObj["completed"] | taskObj["done"] | false;
    
    // Parse deadline if present
    const char* deadline_str = taskObj["deadline"] | taskObj["due_date"] | "";
    if (strlen(deadline_str) > 0) {
      task.has_deadline = true;
      struct tm deadline_tm = {0};
      strptime(deadline_str, "%Y-%m-%dT%H:%M:%S", &deadline_tm);
      task.deadline = mktime(&deadline_tm);
    } else {
      task.has_deadline = false;
    }
    
    task_count++;
  }
  
  Serial.print("Loaded ");
  Serial.print(task_count);
  Serial.println(" tasks");
}

void ui_tasks_update_header(bool wifiConnected) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char time_str[6];
    strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
    lv_label_set_text(time_label, time_str);
  }
  
  lv_label_set_text(wifi_label, wifiConnected ? "WiFi: OK" : "WiFi: --");
  
  // TODO: get actual battery voltage
  lv_label_set_text(battery_label, "Batt: OK");
}
