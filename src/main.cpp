/*
 * ESP-01 Weather & Time Display with WiFi Manager
 * Using Arduino Framework with U8g2 Library
 * For ESP-01 with 0.96" OLED display
 */

#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "time_manager.h"
#include "weather.h"
#include "display.h"

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Disable the blue LED on ESP8266
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // HIGH turns off the LED on most ESP8266 boards
  
  // Add startup delay to see serial output
  delay(2000);
  
  Serial.println("\n\n====================================================");
  Serial.println("ESP-01 Weather & Time Display with WiFi Manager");
  Serial.println("====================================================");
  
  // Initialize display
  u8g2.begin();
  Serial.println("Display initialized");
  
  // Show startup message
  drawConnectingScreen("Starting up...", "Initializing");
  delay(1000);
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Debug info about EEPROM config
  bool isConfigured = (EEPROM.read(CONFIG_FLAG_OFFSET) == 1);
  Serial.print("EEPROM Config Flag: ");
  Serial.println(isConfigured ? "SET (1)" : "NOT SET (0)");
  EEPROM.end();
  
  // Load saved settings for city/state
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
    
    EEPROM.commit();
    EEPROM.end();
    
    Serial.println("Default settings saved to EEPROM");
    delay(1000);
  }
  
  // Clear the configuration flag for testing (REMOVE THIS IN PRODUCTION)
  // Uncomment this line to reset the saved WiFi credentials
  // EEPROM.begin(EEPROM_SIZE);
  // EEPROM.write(CONFIG_FLAG_OFFSET, 0);
  // EEPROM.commit();
  // EEPROM.end();
  // Serial.println("WARNING: Cleared WiFi credentials for testing");
  // delay(2000);
  
  // Display current credentials status
  if (isConfigured) {
    drawConnectingScreen("WiFi configuration", "found");
  } else {
    drawConnectingScreen("No WiFi config", "Will start portal");
  }
  delay(1000);
  
  // Try to connect to saved WiFi
  connectToWifi();
  
  // Disable WiFi status LED again (WiFi connection might have enabled it)
  digitalWrite(LED_BUILTIN, HIGH);
  
  // After WiFi connection, setup NTP
  if (WiFi.status() == WL_CONNECTED) {
    // Setup NTP client
    drawConnectingScreen("Setting up", "NTP time sync");
    setupNTP();
    
    // Get current time
    drawConnectingScreen("Updating", "time & date");
    updateTimeAndDate();
    
    // Fetch weather data
    drawConnectingScreen("Fetching", "weather data");
    fetchWeatherData();
  }
}

void loop() {
  // Get current WiFi mode
  WiFiMode_t currentMode = WiFi.getMode();
  
  // Handle DNS requests if in AP mode
  if (currentMode == WIFI_AP || currentMode == WIFI_AP_STA) {
    dnsServer.processNextRequest();
  }
  
  // Handle web server requests
  server.handleClient();
  
  // Keep LED off (some operations might turn it on)
  digitalWrite(LED_BUILTIN, HIGH);  // HIGH turns off the LED
  
  // AP mode - show configuration portal
  if (currentMode == WIFI_AP) {
    static unsigned long lastRefresh = 0;
    if (millis() - lastRefresh >= 5000) {
      drawConfigMode();
      lastRefresh = millis();
    }
    delay(10);
    return;
  }
  
  // STA mode - handle normal operation
  
  // Check WiFi connection status periodically (every 30 seconds)
  static unsigned long lastWifiCheck = 0;
  static unsigned long lastTimeUpdate = 0;
  static bool initialSetupDone = false;
  
  // If we're connected, perform regular operations
  if (WiFi.status() == WL_CONNECTED) {
    
    // First-time setup after connection
    if (!initialSetupDone) {
      Serial.println("Connection established - performing initial setup");
      
      // Setup NTP
      drawConnectingScreen("Setting up", "NTP time sync");
      setupNTP();
      
      // Get time
      drawConnectingScreen("Updating", "time & date");
      updateTimeAndDate();
      lastTimeUpdate = millis();
      
      // Get weather
      drawConnectingScreen("Fetching", "weather data");
      fetchWeatherData();
      
      initialSetupDone = true;
    }
    
    // Clock updates
    updateCurrentTime();
    
    // Periodic NTP time update every hour
    if (millis() - lastTimeUpdate >= 60 * 60 * 1000) {
      updateTimeAndDate();
      lastTimeUpdate = millis();
    }
    
    // Weather update when needed
    if (millis() - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) {
      fetchWeatherData();
    }
    
    // Periodic WiFi check
    if (millis() - lastWifiCheck >= 30000) {
      // Just update the timestamp - connection is good
      lastWifiCheck = millis();
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
      
      // Simple reconnect approach
      WiFi.reconnect();
      
      // Give it 5 seconds to quickly reconnect
      unsigned long reconnectStart = millis();
      while (millis() - reconnectStart < 5000) {
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("Quick reconnection successful");
          break;
        }
        delay(100);
      }
      
      // If still not connected, do a full reconnect
      if (WiFi.status() != WL_CONNECTED) {
        connectToWifi(); // This handles portal startup if needed
      }
    }
  }
  
  // Display update (regardless of connection status)
  
  // Check if it's time to switch screens
  static unsigned long lastScreenChange = 0;
  if (millis() - lastScreenChange >= SCREEN_SWITCH_INTERVAL) {
    showTimeScreen = !showTimeScreen;
    lastScreenChange = millis();
  }
  
  // Update display
  u8g2.clearBuffer();
  
  if (showTimeScreen) {
    drawTimeScreen();
  } else {
    drawWeatherScreen();
  }
  
  u8g2.sendBuffer();
  
  // Short delay
  delay(50);
}