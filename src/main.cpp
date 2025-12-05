#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "config.h"

// Create display object
Adafruit_7segment display = Adafruit_7segment();

// Track last time we printed network info
unsigned long lastNetworkInfoTime = 0;
const unsigned long networkInfoInterval = 300000;  // 5 minutes in milliseconds

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nChristmas Countdown Timer");

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
    display.print(-1);  // Show error on display
    display.writeDisplay();
    while (1);  // Stop here if no WiFi
  }

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
    Serial.println("\nFailed to obtain time");
  } else {
    Serial.println("\nTime synchronized!");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  }
}

void loop() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
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
  Serial.println(daysUntilChristmas);

  // Print network info every 5 minutes
  unsigned long currentTime = millis();
  if (currentTime - lastNetworkInfoTime >= networkInfoInterval) {
    Serial.println("\n--- Network Status ---");
    Serial.print("Connected to: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.println("---------------------\n");
    lastNetworkInfoTime = currentTime;
  }

  // Update every 60 seconds
  delay(60000);
}
