/*
 * WiFi Manager functions for ESP-01 Weather Display
 * Handles WiFi connection, configuration portal, and credentials storage
 */

#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WString.h>
#include "config.h"
#include "weather.h"

// Structure to hold WiFi credentials
struct WiFiCredentials {
  String ssid;
  String password;
};

// WiFi configuration functions
bool loadWiFiConfig();
void saveWiFiConfig(const char* ssid, const char* password);
void connectToWifi();
void startConfigPortal();
void formatCredentials(); // Format WiFi credentials area in EEPROM
void dumpEEPROMContents();
WiFiCredentials readWiFiCredentialsFromEEPROM(); // Function to read WiFi credentials from EEPROM

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
void saveSettings(String city, String state, unsigned long updateInterval, float timezone, String apiKey, bool is12Hour = false);
String getTimezoneText(float tz);

// Helper functions
void drawConnectingScreen(String message, String submessage);
void drawConfigMode();