#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "config.h"
#include <ESPAsyncWebServer.h>
#include "webpage.h"
#include <Adafruit_MAX1704X.h>

// Create display object
Adafruit_7segment display = Adafruit_7segment();

// Create MAX17048 fuel gauge object
Adafruit_MAX17048 fuelGauge;

// Create web server
AsyncWebServer server(80);

// Track last time we printed network info
unsigned long lastNetworkInfoTime = 0;
const unsigned long networkInfoInterval = 300000;  // 5 minutes in milliseconds

// Track last NTP sync time
unsigned long lastNtpSyncTime = 0;
const unsigned long ntpSyncInterval = 3600000;  // 1 hour in milliseconds

// Track if we have valid time
bool hasValidTime = false;

// Global variables for countdown info
int g_daysUntilChristmas = 0;
int g_currentYear = 0;
int g_currentMonth = 0;
int g_currentDay = 0;
int g_currentHour = 0;
int g_currentMin = 0;
int g_currentSec = 0;
int g_christmasYear = 0;
float g_batteryVoltage = 0.0;
int g_batteryPercent = 0;
String g_batteryStatus = "Unknown";
float g_batteryChargeRate = 0.0;

// Low battery protection
#define LOW_BATTERY_VOLTAGE 3.0  // Voltage threshold for low battery
#define LOW_BATTERY_PERCENT 5    // Percentage threshold for low battery

// Function to try connecting to WiFi and syncing time
void tryWifiAndNtpSync() {
  // Check if we're already connected
  if (WiFi.status() == WL_CONNECTED) {
    // Already connected, just sync time
    Serial.println("Re-syncing time from NTP...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(2000);  // Give it a moment to sync

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      Serial.println("Time re-synchronized!");
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
      hasValidTime = true;
      lastNtpSyncTime = millis();
    } else {
      Serial.println("NTP sync failed, will retry later");
    }
    return;
  }

  // Not connected, try to connect
  Serial.println("\nAttempting to connect to WiFi...");

  for (int i = 0; i < numNetworks; i++) {
    Serial.print("Trying network: ");
    Serial.println(wifiNetworks[i].ssid);

    WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);

    int attempts = 0;
    int maxAttempts = 10;  // 5 seconds per network

    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi reconnected!");
      Serial.print("Connected to: ");
      Serial.println(wifiNetworks[i].ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());

      // Now sync time
      Serial.println("Getting time from NTP server...");
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

      struct tm timeinfo;
      int retries = 0;
      while (!getLocalTime(&timeinfo) && retries < 20) {
        delay(500);
        retries++;
      }

      if (retries < 20) {
        Serial.println("Time synchronized!");
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        hasValidTime = true;
        lastNtpSyncTime = millis();
      }

      return;
    } else {
      Serial.println("\nFailed to connect");
      WiFi.disconnect();
    }
  }

  Serial.println("Could not connect to any WiFi network, will retry later");
}

// Function to enter deep sleep mode
void enterDeepSleep() {
  Serial.println("\n!!! LOW BATTERY WARNING !!!");
  Serial.println("Battery critically low - entering deep sleep to protect battery");
  Serial.print("Voltage: ");
  Serial.print(g_batteryVoltage, 2);
  Serial.println("V");
  Serial.print("Percent: ");
  Serial.print(g_batteryPercent);
  Serial.println("%");

  // Show error on display
  display.clear();
  display.print(0xDEAD);  // Show "dead" pattern
  display.writeDisplay();
  delay(2000);
  display.clear();
  display.writeDisplay();

  Serial.println("Device will wake when USB power is connected or after deep sleep timer");
  Serial.flush();

  // Configure wake on USB power detection (if supported) or timer
  // Wake up every hour to check if power has been restored
  esp_sleep_enable_timer_wakeup(3600ULL * 1000000ULL);  // 1 hour in microseconds

  // Enter deep sleep
  esp_deep_sleep_start();
}

// Function to read battery status from MAX17048
void updateBatteryStatus() {
  // Read voltage from fuel gauge
  g_batteryVoltage = fuelGauge.cellVoltage();

  // Read state of charge percentage (more accurate than voltage-based calculation)
  g_batteryPercent = (int)fuelGauge.cellPercent();

  // Read charge rate (positive = charging, negative = discharging)
  g_batteryChargeRate = fuelGauge.chargeRate();

  // Determine charging status based on charge rate and voltage
  if (g_batteryChargeRate > 0.1) {
    // Positive charge rate = charging
    if (g_batteryPercent >= 99) {
      g_batteryStatus = "Charged";
    } else {
      g_batteryStatus = "Charging";
    }
  } else if (g_batteryChargeRate < -0.1) {
    // Negative charge rate = discharging
    g_batteryStatus = "Discharging";
  } else {
    // Near-zero charge rate
    if (g_batteryPercent >= 95 && g_batteryVoltage >= 4.1) {
      g_batteryStatus = "Charged";
    } else {
      g_batteryStatus = "On Battery";
    }
  }

  // Check for critically low battery (only when discharging)
  if (g_batteryStatus == "Discharging" || g_batteryStatus == "On Battery") {
    if (g_batteryVoltage <= LOW_BATTERY_VOLTAGE || g_batteryPercent <= LOW_BATTERY_PERCENT) {
      enterDeepSleep();
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nChristmas Countdown Timer");

  // Check wake-up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Woke from deep sleep (battery check)");
  }

  // Initialize MAX17048 fuel gauge
  if (!fuelGauge.begin()) {
    Serial.println("Couldn't find MAX17048 fuel gauge!");
    Serial.println("Battery monitoring will be unavailable");
  } else {
    Serial.println("MAX17048 fuel gauge initialized");
    float voltage = fuelGauge.cellVoltage();
    float percent = fuelGauge.cellPercent();

    Serial.print("Battery Voltage: ");
    Serial.print(voltage, 2);
    Serial.println("V");
    Serial.print("Battery Percent: ");
    Serial.print(percent, 1);
    Serial.println("%");

    // If we woke from sleep, check if battery is still too low
    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
      if (voltage <= LOW_BATTERY_VOLTAGE && percent <= LOW_BATTERY_PERCENT) {
        Serial.println("Battery still too low, going back to sleep");
        esp_sleep_enable_timer_wakeup(3600ULL * 1000000ULL);  // Sleep another hour
        esp_deep_sleep_start();
      } else {
        Serial.println("Battery recovered! Continuing normal operation");
      }
    }
  }

  // Initialize the display
  if (!display.begin(displayAddress)) {
    Serial.println("Couldn't find display!");
    while (1);
  }
  Serial.println("Display initialized");
  display.setBrightness(displayBrightness);
  display.print(8888);  // Test pattern
  display.writeDisplay();
  delay(1000);

  // Set hostname before connecting to WiFi
  WiFi.setHostname(hostname);
  Serial.print("Hostname set to: ");
  Serial.println(hostname);

  // Try to connect to WiFi networks with fallback
  bool connected = false;
  display.clear();
  display.writeDisplay();

  for (int i = 0; i < numNetworks && !connected; i++) {
    Serial.print("Trying network: ");
    Serial.println(wifiNetworks[i].ssid);

    WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);

    int dotState = 0;
    int attempts = 0;
    int maxAttempts = 20;  // 10 seconds per network

    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
      delay(500);
      Serial.print(".");
      // Animate dots while connecting
      display.writeDigitNum(0, dotState % 10);
      display.writeDisplay();
      dotState++;
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      Serial.println("\nWiFi connected!");
      Serial.print("Connected to: ");
      Serial.println(wifiNetworks[i].ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nFailed to connect");
      WiFi.disconnect();
    }
  }

  if (!connected) {
    Serial.println("Could not connect to any WiFi network!");
    Serial.println("Continuing without WiFi - using internal RTC only");
    Serial.println("Warning: Time may not be accurate until WiFi connection is established");
  } else {
    // Initialize time from NTP
    Serial.println("Getting time from NTP server...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Wait for time to be set
    struct tm timeinfo;
    int retries = 0;
    while (!getLocalTime(&timeinfo) && retries < 20) {
      delay(500);
      Serial.print(".");
      retries++;
    }

    if (retries >= 20) {
      Serial.println("\nFailed to obtain time from NTP");
      hasValidTime = false;
    } else {
      Serial.println("\nTime synchronized!");
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
      hasValidTime = true;
      lastNtpSyncTime = millis();
    }
  }

  // Setup web server (only if WiFi is connected)
  if (connected) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html);
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
      String json = "{";
      json += "\"days\":" + String(g_daysUntilChristmas) + ",";
      json += "\"christmasYear\":" + String(g_christmasYear) + ",";
      json += "\"year\":" + String(g_currentYear) + ",";
      json += "\"month\":" + String(g_currentMonth) + ",";
      json += "\"day\":" + String(g_currentDay) + ",";
      json += "\"hour\":" + String(g_currentHour) + ",";
      json += "\"min\":" + String(g_currentMin) + ",";
      json += "\"sec\":" + String(g_currentSec) + ",";
      json += "\"ssid\":\"" + String(WiFi.SSID()) + "\",";
      json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
      json += "\"batteryVoltage\":" + String(g_batteryVoltage, 2) + ",";
      json += "\"batteryPercent\":" + String(g_batteryPercent) + ",";
      json += "\"batteryStatus\":\"" + g_batteryStatus + "\"";
      json += "}";
      request->send(200, "application/json", json);
    });

    server.begin();
    Serial.println("\nWeb server started!");
    Serial.print("Visit: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWeb server not started (no WiFi connection)");
  }
}

void loop() {
  // Check if it's time to re-sync NTP (every hour)
  unsigned long currentTime = millis();
  if (currentTime - lastNtpSyncTime >= ntpSyncInterval) {
    tryWifiAndNtpSync();
  }

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    if (!hasValidTime) {
      Serial.println("No valid time available - trying to sync...");
      tryWifiAndNtpSync();
    }
    display.print(0);
    display.writeDisplay();
    delay(1000);
    return;
  }

  // Calculate days until Christmas
  int currentYear = timeinfo.tm_year + 1900;
  int currentMonth = timeinfo.tm_mon + 1;  // tm_mon is 0-11
  int currentDay = timeinfo.tm_mday;

  // Determine which Christmas to count down to
  int christmasYear = currentYear;
  if (currentMonth == 12 && currentDay > 25) {
    christmasYear = currentYear + 1;
  }

  // Create time structure for Christmas
  struct tm christmas = {0};
  christmas.tm_year = christmasYear - 1900;
  christmas.tm_mon = 11;  // December (0-indexed)
  christmas.tm_mday = 25;
  christmas.tm_hour = 0;
  christmas.tm_min = 0;
  christmas.tm_sec = 0;

  // Convert to time_t
  time_t now = mktime(&timeinfo);
  time_t christmasTime = mktime(&christmas);

  // Calculate difference in seconds and convert to days
  double secondsUntilChristmas = difftime(christmasTime, now);
  int daysUntilChristmas = (int)(secondsUntilChristmas / 86400);

  // Update global variables for web server
  g_daysUntilChristmas = daysUntilChristmas;
  g_currentYear = currentYear;
  g_currentMonth = currentMonth;
  g_currentDay = currentDay;
  g_currentHour = timeinfo.tm_hour;
  g_currentMin = timeinfo.tm_min;
  g_currentSec = timeinfo.tm_sec;
  g_christmasYear = christmasYear;

  // Update battery status
  updateBatteryStatus();

  // Handle Christmas day specially
  if (daysUntilChristmas == 0 && currentMonth == 12 && currentDay == 25) {
    // It's Christmas! Show 0 and blink
    display.print(0);
    display.drawColon(millis() % 1000 < 500);
  } else {
    // Display the countdown
    display.print(daysUntilChristmas);
    display.drawColon(false);
  }

  display.writeDisplay();

  // Debug output
  Serial.print("Today: ");
  Serial.print(currentMonth);
  Serial.print("/");
  Serial.print(currentDay);
  Serial.print("/");
  Serial.print(currentYear);
  Serial.print(" ");
  Serial.print(timeinfo.tm_hour);
  Serial.print(":");
  if (timeinfo.tm_min < 10) Serial.print("0");
  Serial.print(timeinfo.tm_min);
  Serial.print(":");
  if (timeinfo.tm_sec < 10) Serial.print("0");
  Serial.print(timeinfo.tm_sec);
  Serial.print(" - Days until Christmas ");
  Serial.print(christmasYear);
  Serial.print(": ");
  Serial.print(daysUntilChristmas);
  Serial.print(" - Battery: ");
  Serial.print(g_batteryVoltage, 2);
  Serial.print("V (");
  Serial.print(g_batteryPercent);
  Serial.print("%) - ");
  Serial.println(g_batteryStatus);

  // Print network info every 5 minutes
  if (currentTime - lastNetworkInfoTime >= networkInfoInterval) {
    Serial.println("\n--- Network Status ---");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Connected to: ");
      Serial.println(WiFi.SSID());
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.print("Signal Strength: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
      Serial.print("Last NTP sync: ");
      Serial.print((currentTime - lastNtpSyncTime) / 1000);
      Serial.println(" seconds ago");
    } else {
      Serial.println("WiFi: Not Connected");
      Serial.println("Running on internal RTC");
      if (hasValidTime) {
        Serial.print("Last NTP sync: ");
        Serial.print((currentTime - lastNtpSyncTime) / 1000);
        Serial.println(" seconds ago");
      } else {
        Serial.println("Warning: Time may not be accurate");
      }
    }
    Serial.println("---------------------\n");
    lastNetworkInfoTime = currentTime;
  }

  // Update every 60 seconds
  delay(60000);
}
