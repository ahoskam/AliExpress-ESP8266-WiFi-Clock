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
#define CONFIG_FLAG 1  // Value indicating valid WiFi configuration
// CONFIG_FLAG_OFFSET+1 and +2 are reserved for SSID and password lengths
#define CITY_OFFSET 132
#define STATE_OFFSET 182
#define UPDATE_INTERVAL_OFFSET 200
#define TIMEZONE_OFFSET 210
#define API_KEY_OFFSET 220
#define TIME_FORMAT_OFFSET 270  // New offset for time format preference
#define TEMP_UNIT_OFFSET 271  // Offset for temperature unit preference (C/F)
#define USE_DST_OFFSET 255 // New offset for DST setting

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
extern String API_KEY;
extern String UNITS;  // Changed from const to allow dynamic switching
extern String cityName;
extern String stateName;
extern bool useMetricUnits; // true for Celsius, false for Fahrenheit

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
extern int currentMinute;
extern int currentSecond;
extern int currentDay;
extern int currentMonth;
extern int currentYear;
extern int currentDayOfWeek;
extern String timeString;
extern String dateString;
extern String timezoneString;
extern bool timeInitialized;
extern unsigned long lastTimeUpdate;
extern unsigned long lastSecondUpdate;
extern float timezone; // UTC offset in hours (e.g., -5 for EST)
extern bool use12HourFormat; // true for 12-hour format with AM/PM, false for 24-hour format
extern bool useDST; // Flag to enable/disable DST calculations
extern long driftCorrection; // Milliseconds of drift correction per hour
extern unsigned long lastNtpTimestamp; // Last NTP time in seconds
extern unsigned long lastNtpMillis; // millis() value at last NTP sync

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
extern WeatherDay forecast[5];

// Weather icon bitmaps
extern const uint8_t sunny_icon[8];
extern const uint8_t partly_cloudy_icon[8];
extern const uint8_t cloudy_icon[8];
extern const uint8_t foggy_icon[8];
extern const uint8_t rainy_icon[8];
extern const uint8_t snowy_icon[8];

#endif // CONFIG_H
