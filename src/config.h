/*
 * Configuration file for ESP-01 Weather Display
 * Contains pin definitions, constants, and global variables
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Define pins for I2C on ESP-01
#define SDA_PIN 0  // GPIO0
#define SCL_PIN 2  // GPIO2

// EEPROM size and offset
#define EEPROM_SIZE 512
#define WIFI_SSID_OFFSET 0
#define WIFI_PASS_OFFSET 32
#define CONFIG_FLAG_OFFSET 128
#define CITY_OFFSET 130
#define STATE_OFFSET 180
#define UPDATE_INTERVAL_OFFSET 200
#define TIMEZONE_OFFSET 210

// Configuration portal constants
extern const char* AP_NAME;
extern const char* AP_PASSWORD;
extern const byte DNS_PORT;
extern IPAddress apIP;

// Display instance
extern U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2;

// Web server and DNS server
extern ESP8266WebServer server;
extern DNSServer dnsServer;

// NTP Client
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;

// OpenWeatherMap settings
extern const String API_KEY;
extern const String UNITS;
extern String cityName;
extern String stateName;

// Screen switching timing
extern const unsigned long SCREEN_SWITCH_INTERVAL;
extern unsigned long lastScreenChange;
extern bool showTimeScreen;

// Weather update interval (configurable)
extern unsigned long WEATHER_UPDATE_INTERVAL;

// Time variables
extern int hours;
extern int minutes;
extern int seconds;
extern int dayOfMonth;
extern int month;
extern int year;
extern String dayOfWeekStr;
extern String monthStr;
extern int currentHour;
extern bool timeInitialized;
extern unsigned long lastTimeUpdate;
extern unsigned long lastSecondUpdate;
extern float timezone; // UTC offset in hours (e.g., -5 for EST)

// Weather variables
extern int currentTemp;
extern int lowTemp;
extern int highTemp;
extern String currentCondition;
extern int humidity;
extern int sunriseHour;
extern int sunriseMinute;
extern int sunsetHour;
extern int sunsetMinute;
extern unsigned long lastWeatherUpdate;

// Weather forecast data structure
struct WeatherDay {
  String day;
  int temp;
  int lowTemp;
  byte iconType; // 0=sunny, 1=partly cloudy, 2=cloudy, 3=foggy, 4=rainy, 5=snowy
};

// Weather forecast data
extern WeatherDay forecast[6];

// Weather icon bitmaps
extern const uint8_t sunny_icon[8];
extern const uint8_t partly_cloudy_icon[8];
extern const uint8_t cloudy_icon[8];
extern const uint8_t foggy_icon[8];
extern const uint8_t rainy_icon[8];
extern const uint8_t snowy_icon[8];

#endif // CONFIG_H
