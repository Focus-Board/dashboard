#include "ui_journal.h"
#include "ui_header.h"
#include <ArduinoJson.h>
#include <time.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define HEADER_HEIGHT 30
#define COLUMN_COUNT 3
#define COLUMN_PADDING 5
#define ENTRY_PADDING 8
#define ENTRY_SPACING 12
#define MAX_ENTRIES_PER_COLUMN 3

static lv_obj_t *screen_journal = NULL;
static ui_header_t header;
static lv_obj_t *journal_container = NULL;
static lv_obj_t *columns[COLUMN_COUNT];
static lv_obj_t *column_labels[COLUMN_COUNT];

#define MAX_ENTRIES 50
struct JournalEntry {
  String content;
  time_t created;
  int day_of_week;
};

static JournalEntry entries[MAX_ENTRIES];
static int entry_count = 0;

static void clear_column(int col);
static void add_entry_to_column(int col, JournalEntry &entry);
static int get_day_of_week_from_monday(struct tm *timeinfo);

void ui_journal_init() {
  screen_journal = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen_journal, lv_color_white(), 0);
  
  ui_header_create(screen_journal, &header);
  
  journal_container = lv_obj_create(screen_journal);
  lv_obj_set_size(journal_container, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT);
  lv_obj_set_pos(journal_container, 0, HEADER_HEIGHT);
  lv_obj_set_style_bg_color(journal_container, lv_color_white(), 0);
  lv_obj_set_style_border_width(journal_container, 0, 0);
  lv_obj_set_style_pad_all(journal_container, COLUMN_PADDING, 0);
  
  int col_width = (SCREEN_WIDTH - (COLUMN_PADDING * 4)) / COLUMN_COUNT;
  
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  
  
  for (int i = 0; i < COLUMN_COUNT; i++) {
    int x = COLUMN_PADDING + i * (col_width + COLUMN_PADDING);
    
    columns[i] = lv_obj_create(journal_container);
    lv_obj_set_size(columns[i], col_width, SCREEN_HEIGHT - HEADER_HEIGHT);
    lv_obj_set_pos(columns[i], x, 0);
    lv_obj_set_style_bg_color(columns[i], lv_color_white(), 0);
    lv_obj_set_style_border_width(columns[i], 1, 0);
    lv_obj_set_style_border_color(columns[i], lv_color_black(), 0);
    lv_obj_set_style_pad_all(columns[i], ENTRY_PADDING, 0);
    lv_obj_set_style_radius(columns[i], 0, 0);
    lv_obj_set_flex_flow(columns[i], LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(columns[i], LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(columns[i], ENTRY_SPACING, 0);
  }
}

void ui_journal_show() {
  lv_screen_load(screen_journal);
  ui_header_update_time(&header);
  
  for (int i = 0; i < COLUMN_COUNT; i++) {
    clear_column(i);
  }
  
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  int today = get_day_of_week_from_monday(&timeinfo);
  
  int col_days[3] = {
    today,
    (today - 1 + 7) % 7,
    (today - 2 + 7) % 7
  };
  
  int col_counts[3] = {0, 0, 0};
  
  for (int i = entry_count - 1; i >= 0; i--) {
    for (int col = 0; col < COLUMN_COUNT; col++) {
      if (entries[i].day_of_week == col_days[col] && 
          col_counts[col] < MAX_ENTRIES_PER_COLUMN) {
        add_entry_to_column(col, entries[i]);
        col_counts[col]++;
        break;
      }
    }
  }
}

void ui_journal_update_entries(String jsonData) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }
  
  entry_count = 0;
  JsonArray entriesArray = doc.as<JsonArray>();
  
  for (JsonObject entryObj : entriesArray) {
    if (entry_count >= MAX_ENTRIES) break;
    
    JournalEntry &entry = entries[entry_count];
    entry.content = entryObj["description"] | "";
    
    const char* created_str = entryObj["date"] | "";
    if (strlen(created_str) > 0) {
      struct tm created_tm = {0};
      strptime(created_str, "%Y-%m-%dT%H:%M:%S", &created_tm);
      entry.created = mktime(&created_tm);
      entry.day_of_week = get_day_of_week_from_monday(&created_tm);
    }
    
    entry_count++;
  }
  
  Serial.print("Loaded ");
  Serial.print(entry_count);
  Serial.println(" journal entries");
}

void ui_journal_update_header(bool wifiConnected) {
  ui_header_update_time(&header);
  ui_header_set_left_text(&header, "Focusboard: Journal");
  ui_header_set_right_text(&header, "Batt: 100%");
}

static void clear_column(int col) {
  lv_obj_clean(columns[col]);
}

static void add_entry_to_column(int col, JournalEntry &entry) {
  int col_width = (SCREEN_WIDTH - (COLUMN_PADDING * 4)) / COLUMN_COUNT;
  
  lv_obj_t *entry_container = lv_obj_create(columns[col]);
  lv_obj_set_width(entry_container, col_width - (ENTRY_PADDING * 2));
  lv_obj_set_height(entry_container, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(entry_container, lv_color_hex(0xF0F0F0), 0);
  lv_obj_set_style_border_width(entry_container, 1, 0);
  lv_obj_set_style_border_color(entry_container, lv_color_hex(0xCCCCCC), 0);
  lv_obj_set_style_pad_all(entry_container, 8, 0);
  lv_obj_set_style_radius(entry_container, 0, 0);
  
  lv_obj_t *timestamp_label = lv_label_create(entry_container);
  struct tm created_tm;
  localtime_r(&entry.created, &created_tm);
  char time_str[16];
  strftime(time_str, sizeof(time_str), "%H:%M", &created_tm);
  lv_label_set_text(timestamp_label, time_str);
  lv_obj_set_pos(timestamp_label, 0, 0);
  lv_obj_set_style_text_font(timestamp_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(timestamp_label, lv_color_hex(0x888888), 0);
  
  lv_obj_t *content_label = lv_label_create(entry_container);
  lv_label_set_text(content_label, entry.content.c_str());
  lv_obj_set_pos(content_label, 0, 20);
  lv_obj_set_width(content_label, col_width - (ENTRY_PADDING * 4) - 16);
  lv_obj_set_style_text_font(content_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(content_label, lv_color_black(), 0);
  lv_label_set_long_mode(content_label, LV_LABEL_LONG_WRAP);
  
  int content_height = lv_obj_get_height(content_label);
  lv_obj_set_height(entry_container, 20 + content_height + 16);
}

static int get_day_of_week_from_monday(struct tm *timeinfo) {
  int dow = timeinfo->tm_wday;
  return (dow == 0) ? 6 : dow - 1;
}
