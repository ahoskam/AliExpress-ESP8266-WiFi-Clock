/*
 * WiFi Manager functions for ESP-01 Weather Display
 * Handles WiFi connection, configuration portal, and credentials storage
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include "config.h"

// WiFi configuration functions
bool loadWiFiConfig();
void saveWiFiConfig(const char* ssid, const char* password);
void connectToWifi();
void startConfigPortal();
void formatCredentials(); // Format WiFi credentials area in EEPROM
void dumpEEPROMContents();

// Web server routes
void setupWebServer();
void handleRoot();
void handleWifiScan();
void handleSave();
void handleNotFound();

// Settings page handlers
void handleSettings();
void handleSettingsSave();
void loadSettings();
void saveSettings(String city, String state, unsigned long updateInterval, float timezone);
String getTimezoneText(float tz);

// Helper functions
void drawConnectingScreen(String message, String submessage);
void drawConfigMode();

#endif // WIFI_MANAGER_H