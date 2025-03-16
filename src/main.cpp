/*
 * ESP-01 Weather & Time Display with WiFi Manager
 * Using Arduino Framework with U8g2 Library
 * For ESP-01 with 0.96" OLED display
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "config.h"  // Include the main configuration that has all necessary declarations
#include "weather.h" // Include weather functions
#include "wifi_manager.h" // Include WiFi manager functions
#include "time_manager.h"
#include "display.h"

// Define EEPROM test offset (not used in main program)
#define TEST_OFFSET 0

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give serial time to initialize
  
  Serial.println("Starting ESP Weather Display");
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Initialize display
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tr);
  
  // Check if WiFi credentials exist
  uint8_t configFlag = EEPROM.read(CONFIG_FLAG_OFFSET);
  uint8_t ssidLen = EEPROM.read(CONFIG_FLAG_OFFSET + 1);
  uint8_t passLen = EEPROM.read(CONFIG_FLAG_OFFSET + 2);
  
  if (configFlag != CONFIG_FLAG) {
    Serial.println("No valid WiFi configuration found");
    startConfigPortal();
    return;
  }
  
  // Try to connect with stored credentials
  if (!loadWiFiConfig()) {
    Serial.println("Failed to connect with stored credentials");
    startConfigPortal();
    return;
  }
  
  // Show startup message
  drawConnectingScreen("Starting up...", "Initializing");
  delay(1000);
  
  // Debug EEPROM WiFi credentials storage
  Serial.println("\n----- WiFi Credentials Debug -----");
  Serial.println("Reading WiFi configuration...");
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Debug info about EEPROM config
  bool isConfigured = (EEPROM.read(CONFIG_FLAG_OFFSET) == CONFIG_FLAG);
  Serial.print("EEPROM Config Flag: ");
  Serial.println(isConfigured ? "SET (1)" : "NOT SET (0)");
  
  EEPROM.end();
  
  // Load saved settings for city/state
  Serial.println("\n----- Loading Settings -----");
  drawConnectingScreen("Starting up...", "Loading settings");
  loadSettings();
  
  // Check if city name is valid, if not reset it
  if (cityName.length() == 0 || cityName == "_") {
    Serial.println("City name is invalid, resetting to default and saving");
    cityName = "New York";
    stateName = "NY";
    timezone = -5.0f; // Set default timezone to Eastern Time
    
    // Save settings to EEPROM directly
    EEPROM.begin(EEPROM_SIZE);
    
    // Save city name
    for (size_t i = 0; i < 50; i++) {
      if (i < cityName.length()) {
        EEPROM.write(CITY_OFFSET + i, cityName[i]);
      } else {
        EEPROM.write(CITY_OFFSET + i, 0);
      }
    }
    
    // Save state code
    for (size_t i = 0; i < 2; i++) {
      if (i < stateName.length()) {
        EEPROM.write(STATE_OFFSET + i, stateName[i]);
      } else {
        EEPROM.write(STATE_OFFSET + i, 0);
      }
    }
    
    // Save update interval
    byte intervalBytes[4];
    memcpy(intervalBytes, &WEATHER_UPDATE_INTERVAL, 4);
    for (int i = 0; i < 4; i++) {
      EEPROM.write(UPDATE_INTERVAL_OFFSET + i, intervalBytes[i]);
    }
    
    // Save timezone
    byte timezoneBytes[4];
    memcpy(timezoneBytes, &timezone, 4);
    for (int i = 0; i < 4; i++) {
      EEPROM.write(TIMEZONE_OFFSET + i, timezoneBytes[i]);
    }
    
    // Save API key
    for (size_t i = 0; i < 50; i++) {
      if (i < API_KEY.length()) {
        EEPROM.write(API_KEY_OFFSET + i, API_KEY[i]);
      } else {
        EEPROM.write(API_KEY_OFFSET + i, 0);
      }
    }
    
    EEPROM.commit();
    EEPROM.end();
    
    Serial.println("Default settings saved to EEPROM");
    delay(1000);
  }
  
  // Display current credentials status
  if (isConfigured) {
    drawConnectingScreen("WiFi configuration", "found");
  } else {
    drawConnectingScreen("No WiFi config", "Will start portal");
  }
  delay(1000);
  
  // Try to connect to saved WiFi with more detailed output
  Serial.println("\n----- WiFi Connection -----");
  Serial.println("Starting WiFi connection process...");
  connectToWifi();
  
  // After WiFi connection, setup NTP
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected successfully, setting up services");
    
    // Setup NTP client
    Serial.println("\n----- Time Synchronization -----");
    drawConnectingScreen("Setting up", "NTP time sync");
    setupNTP();
    
    // Get current time
    drawConnectingScreen("Updating", "time & date");
    bool timeSuccess = updateTimeAndDate();
    if (!timeSuccess) {
      Serial.println("[Time] Initial time update failed, will retry later");
    } else {
      Serial.println("[Time] Initial time update successful");
      timeInitialized = true;
      lastSecondUpdate = millis();
    }
    
    // Fetch weather data
    drawConnectingScreen("Fetching", "weather data");
    bool weatherSuccess = Weather::fetchWeatherData();
    if (!weatherSuccess) {
      Serial.println("[Weather] Initial weather update failed, will retry later");
    }
  } else {
    Serial.println("WiFi not connected after initialization");
  }
  
  Serial.println("\n----- Setup Complete -----");
}

void loop() {
  // Get current WiFi mode
  WiFiMode_t currentMode = WiFi.getMode();
  bool isAPActive = (currentMode == WIFI_AP || currentMode == WIFI_AP_STA) && WiFi.softAPgetStationNum() > 0;
  
  // Handle DNS requests if in AP mode
  if (currentMode == WIFI_AP || currentMode == WIFI_AP_STA) {
    dnsServer.processNextRequest();
  }
  
  // Handle web server requests
  server.handleClient();
  
  // AP mode with active connections - focus on serving the portal
  if (isAPActive) {
    static unsigned long lastRefresh = 0;
    if (millis() - lastRefresh >= 5000) {
      drawConfigMode();
      lastRefresh = millis();
    }
    delay(10);
    return;
  }
  
  // AP mode without connections OR STA+AP mode without active clients
  if (currentMode == WIFI_AP || 
     (currentMode == WIFI_AP_STA && WiFi.status() != WL_CONNECTED && !isAPActive)) {
    // If we're in AP mode only (or AP+STA but not connected), periodically refresh the display
    static unsigned long lastRefresh = 0;
    if (millis() - lastRefresh >= 5000) {
      drawConfigMode();
      lastRefresh = millis();
    }
    delay(10);
    return;
  }
  
  // STA mode or AP+STA mode with active connection - handle normal operation
  
  // Check WiFi connection status periodically (every 30 seconds)
  static unsigned long lastWifiCheck = 0;
  static unsigned long lastTimeUpdate = 0;
  static bool initialSetupDone = false;
  
  // If we're connected, perform regular operations
  if (WiFi.status() == WL_CONNECTED) {
    
    // First-time setup after connection
    if (!initialSetupDone) {
      Serial.println("Connection established - performing initial setup");
      
      // Skip NTP setup and time update since it was already done in setup()
      
      // Get weather
      drawConnectingScreen("Fetching", "weather data");
      bool weatherSuccess = Weather::fetchWeatherData();
      if (!weatherSuccess) {
        Serial.println("[Weather] Initial weather update failed, will retry later");
      }
      
      initialSetupDone = true;
      // Initialize lastTimeUpdate to current time so we don't immediately trigger another update
      lastTimeUpdate = millis();
    }
    
    // Clock updates
    updateCurrentTime();
    
    // Periodic NTP time update every 10 minutes (reduced from 30 minutes for better accuracy)
    if (millis() - lastTimeUpdate >= 10 * 60 * 1000) {
      Serial.println("[Time] Time update initiated...");
      bool timeUpdateSuccess = updateTimeAndDate();
      if (timeUpdateSuccess) {
        Serial.println("[Time] Time update successful");
        // Format time and date strings for display
        char timeStr[16];
        formatTimeString(timeStr, hours, minutes, use12HourFormat);
        
        char dateStr[32]; // Increased from 20 to 32 to prevent buffer overflow
        sprintf(dateStr, "%s %s %d, %d", dayOfWeekStr.c_str(), monthStr.c_str(), dayOfMonth, year);
        
        Serial.print("[Time] Current time: ");
        Serial.print(timeStr);
        Serial.print(" ");
        Serial.println(dateStr);
      } else {
        Serial.println("[Time] Time update failed");
      }
      lastTimeUpdate = millis();
    }
    
    // Weather update when needed
    if (millis() - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) {
      Serial.println("[Weather] Weather update initiated...");
      bool weatherUpdateSuccess = Weather::fetchWeatherData();
      if (weatherUpdateSuccess) {
        Serial.println("[Weather] Weather update successful");
        Serial.println("[Weather] Current temp: " + String(currentTemp) + "°F, Condition: " + currentCondition);
      } else {
        Serial.println("[Weather] Weather update failed");
      }
    }
    
    // Periodic WiFi check
    if (millis() - lastWifiCheck >= 30000) {
      // Just update the timestamp - connection is good
      lastWifiCheck = millis();
      Serial.println("[WiFi] Connection check: Connected to " + WiFi.SSID() + " (" + WiFi.localIP().toString() + ")");
    }
  }
  // Not connected - try to reconnect
  else {
    // Only attempt reconnection every 30 seconds
    if (millis() - lastWifiCheck >= 30000) {
      lastWifiCheck = millis();
      
      Serial.println("WiFi disconnected - attempting reconnection");
      initialSetupDone = false;
      
      // Try to reconnect
      drawConnectingScreen("Reconnecting", "to WiFi");
      
      // First try a simple reconnect
      WiFi.reconnect();
      
      // Give it 10 seconds to quickly reconnect
      unsigned long reconnectStart = millis();
      bool quickReconnected = false;
      
      while (millis() - reconnectStart < 10000) {
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("Quick reconnection successful");
          quickReconnected = true;
          break;
        }
        // Visual feedback
        if ((millis() - reconnectStart) % 250 < 50) {
          String dots = String((millis() - reconnectStart) / 250 % 4, '.');
          drawConnectingScreen("Reconnecting", dots);
        }
        delay(100);
      }
      
      // If quick reconnect failed, try a full reconnect
      if (!quickReconnected) {
        Serial.println("Quick reconnect failed, trying full reconnect");
        drawConnectingScreen("Quick reconnect failed", "Trying full reconnect");
        delay(1000);
        connectToWifi(); // This handles portal startup if needed
      }
    }
  }
  
  // Display update (regardless of connection status)
  
  // Check if it's time to switch screens
  static unsigned long lastScreenChange = 0;
  static byte currentScreen = 0; // 0 = time, 1 = current weather, 2 = forecast
  
  if (millis() - lastScreenChange >= SCREEN_SWITCH_INTERVAL) {
    currentScreen = (currentScreen + 1) % 3; // Cycle through 3 screens
    lastScreenChange = millis();
  }
  
  // Update display
  u8g2.clearBuffer();
  
  switch (currentScreen) {
    case 0:
      drawTimeScreen();
      break;
    case 1:
      drawCurrentWeatherScreen();
      break;
    case 2:
      drawForecastScreen();
      break;
  }
  
  u8g2.sendBuffer();
  
  // Short delay
  delay(50);
}