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
  // Handle DNS requests
  dnsServer.processNextRequest();
  
  // Handle web server requests
  server.handleClient();
  
  // Keep LED off (some operations might turn it on)
  static unsigned long lastLedCheck = 0;
  if (millis() - lastLedCheck >= 1000) { // Check every second
    digitalWrite(LED_BUILTIN, HIGH);  // HIGH turns off the LED
    lastLedCheck = millis();
  }
  
  // If in AP mode, keep showing the configuration screen
  if (WiFi.getMode() == WIFI_AP) {
    // Refresh the configuration screen periodically
    static unsigned long lastRefresh = 0;
    if (millis() - lastRefresh >= 5000) {
      drawConfigMode();
      lastRefresh = millis();
    }
    return;
  }
  
  // Check WiFi connection status periodically
  static unsigned long lastWifiCheck = 0;
  static bool webServerChecked = false;
  
  if (millis() - lastWifiCheck >= 30000) { // Check every 30 seconds
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WARNING: WiFi connection lost, attempting to reconnect");
      // Try a quick reconnect first
      int reconnectAttempts = 0;
      while (WiFi.status() != WL_CONNECTED && reconnectAttempts < 6) {
        drawConnectingScreen("Connection lost", "Reconnecting...");
        delay(1000);
        reconnectAttempts++;
      }
      
      // If still not connected, try full reconnect
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnection failed, trying full reconnect");
        webServerChecked = false; // Reset flag to check web server again after reconnect
        connectToWifi();
      }
    } else if (!webServerChecked) {
      // Ensure web server is running in STA mode (only once after connection)
      Serial.println("Ensuring web server is running in STA mode");
      server.stop();
      delay(500);
      setupWebServer();
      server.begin();
      Serial.println("Web server restarted at http://" + WiFi.localIP().toString());
      webServerChecked = true;
    }
    lastWifiCheck = millis();
  }
  
  // Update the internal clock
  updateCurrentTime();
  
  // Check if WiFi is connected and time not initialized
  if (WiFi.status() == WL_CONNECTED && !timeInitialized) {
    updateTimeAndDate();
  }
  
  // Update time from NTP every hour
  static unsigned long lastTimeUpdate = 0;
  if (millis() - lastTimeUpdate >= 60 * 60 * 1000) { // 1 hour
    updateTimeAndDate();
    lastTimeUpdate = millis();
  }
  
  // Update weather data periodically
  if (millis() - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) {
    Serial.println("Time to update weather data");
    if (fetchWeatherData()) {
      Serial.println("Weather data updated successfully");
    } else {
      Serial.println("Failed to update weather data");
    }
  }
  
  // Check if it's time to switch screens
  unsigned long currentMillis = millis();
  if (currentMillis - lastScreenChange >= SCREEN_SWITCH_INTERVAL) {
    showTimeScreen = !showTimeScreen;
    lastScreenChange = currentMillis;
  }
  
  // Update display
  u8g2.clearBuffer();
  
  // Draw the appropriate screen
  if (showTimeScreen) {
    drawTimeScreen();
  } else {
    drawWeatherScreen();
  }
  
  // Send buffer to display
  u8g2.sendBuffer();
  
  // Short delay to prevent too frequent updates
  delay(100);
}