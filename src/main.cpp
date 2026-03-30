#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <lvgl.h>
#include "e1002_display.h"
#include "wifi_credentials.h"
#include "ui_calendar.h"
#include "ui_tasks.h"

const int BUTTON_TOGGLE = 2;

const unsigned long REFRESH_INTERVAL = 30 * 60 * 1000; // 30 minutes
const unsigned long DEBOUNCE_DELAY = 120;
const unsigned long FULL_REFRESH_COOLDOWN = 1500;

// API stuff
const char* API_BASE = "http://72.240.83.248:8000";
const char* EVENTS_ENDPOINT = "/api/calendar/events?days=7&days_back=7";
const char* TASKS_ENDPOINT = "/api/calendar/tasks"; // TODO: update when

const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = -5 * 3600; 
const int DAYLIGHT_OFFSET_SEC = 3600;

e1002_driver_t e1002_driver;
enum Page { PAGE_CALENDAR, PAGE_TASKS };
Page currentPage = PAGE_CALENDAR;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long lastFullRefreshTime = 0;
unsigned long lastDataFetchTime = 0;

String lastEventsData = "";
String lastTasksData = "";
bool dataChanged = false;

bool wifiConnected = false;

void setupWiFi();
void syncTime();
bool fetchCalendarData();
bool fetchTasksData();
void switchPage(Page newPage);
bool buttonPressed();
void updateDisplay();
void updateHeader();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("FocusBoard");
  Serial.println("==========");
  
  Serial.println("Initializing display...");
  e1002_display_init(&e1002_driver);
  
  pinMode(BUTTON_TOGGLE, INPUT_PULLUP);
  
  setupWiFi();
  
  if (wifiConnected) {
    syncTime();
  }
  
  ui_calendar_init();
  ui_tasks_init();
  
  if (wifiConnected) {
    fetchCalendarData();
    fetchTasksData();
  }
  
  switchPage(PAGE_CALENDAR);
  
  Serial.println("Setup complete!");
}

void loop() {
  lv_timer_handler();
  
  if (buttonPressed()) {
    Page newPage = (currentPage == PAGE_CALENDAR) ? PAGE_TASKS : PAGE_CALENDAR;
    switchPage(newPage);
  }
  
  if (wifiConnected && (millis() - lastDataFetchTime > REFRESH_INTERVAL)) {
    Serial.println("Fetching updated data...");
    bool eventsChanged = fetchCalendarData();
    bool tasksChanged = fetchTasksData();
    
    if (eventsChanged || tasksChanged) {
      dataChanged = true;
      updateDisplay();
    }
    
    lastDataFetchTime = millis();
  }
  
  // Update time in header every minute
  static unsigned long lastTimeUpdate = 0;
  if (millis() - lastTimeUpdate > 60000) {
    updateHeader();
    lastTimeUpdate = millis();
  }
  
  if (e1002_display_should_refresh(&e1002_driver)) {
    Serial.println("Refreshing e-paper display...");
    e1002_display_refresh(&e1002_driver);
    Serial.println("Display refresh complete");
  }
  
  delay(10);
}

void setupWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println("\nWiFi connection failed!");
  }
}

void syncTime() {
  Serial.println("Syncing time with NTP server...");
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("Time synchronized successfully");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  } else {
    Serial.println("Failed to sync time");
  }
}

bool fetchCalendarData() {
  if (!wifiConnected) return false;
  
  HTTPClient http;
  String url = String(API_BASE) + String(EVENTS_ENDPOINT);
  
  Serial.print("Fetching events from: ");
  Serial.println(url);
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    // Check if data changed
    bool changed = (payload != lastEventsData);
    if (changed) {
      lastEventsData = payload;
      Serial.println("Events data changed");
      
      ui_calendar_update_events(payload);
    } else {
      Serial.println("Events data unchanged");
    }
    
    http.end();
    return changed;
  } else {
    Serial.print("HTTP GET failed, error: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}

bool fetchTasksData() {
  if (!wifiConnected) return false;
  
  HTTPClient http;
  String url = String(API_BASE) + String(TASKS_ENDPOINT);
  
  Serial.print("Fetching tasks from: ");
  Serial.println(url);
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    // Check if data changed
    bool changed = (payload != lastTasksData);
    if (changed) {
      lastTasksData = payload;
      Serial.println("Tasks data changed");
      
      ui_tasks_update_tasks(payload);
    } else {
      Serial.println("Tasks data unchanged");
    }
    
    http.end();
    return changed;
  } else {
    Serial.print("HTTP GET failed, error: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}

void switchPage(Page newPage) {
  if (millis() - lastFullRefreshTime < FULL_REFRESH_COOLDOWN) {
    Serial.println("Refresh cooling down, skipping page switch");
    return;
  }
  
  currentPage = newPage;
  
  Serial.print("Switching to page: ");
  Serial.println(newPage == PAGE_CALENDAR ? "CALENDAR" : "TASKS");
  
  // Clear screen
  e1002_driver.epd->fillScreen(TFT_WHITE);
  
  if (newPage == PAGE_CALENDAR) {
    ui_calendar_show();
  } else {
    ui_tasks_show();
  }
  
  e1002_display_schedule_refresh(&e1002_driver);
  
  lastFullRefreshTime = millis();
}

bool buttonPressed() {
  bool currentState = digitalRead(BUTTON_TOGGLE);
  
  if (lastButtonState == HIGH && currentState == LOW &&
      (millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    lastDebounceTime = millis();
    lastButtonState = currentState;
    return true;
  }
  
  lastButtonState = currentState;
  return false;
}

void updateDisplay() {
  if (currentPage == PAGE_CALENDAR) {
    ui_calendar_show();
  } else {
    ui_tasks_show();
  }
  
  e1002_display_schedule_refresh(&e1002_driver);
}

void updateHeader() {
  if (currentPage == PAGE_CALENDAR) {
    ui_calendar_update_header(wifiConnected);
  } else {
    ui_tasks_update_header(wifiConnected);
  }
  
  e1002_display_schedule_refresh(&e1002_driver);
}
