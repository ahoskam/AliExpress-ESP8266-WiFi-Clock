/*
 * Implementation of the global variables declared in config.h
 */

#include "config.h"

// Configuration portal constants
const char* AP_NAME = "ESP-Weather";  // SSID of the configuration AP
const char* AP_PASSWORD = "weatherconfig";  // Password for the AP
const byte DNS_PORT = 53;  // DNS server port
IPAddress apIP(192, 168, 4, 1);  // IP address for AP mode

// Display instance
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R2, SCL_PIN, SDA_PIN, U8X8_PIN_NONE);

// Web server and DNS server
ESP8266WebServer server(80);
DNSServer dnsServer;

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// OpenWeatherMap settings
const String API_KEY = "61638ec563f3f9e230dbeb2e3a83a507";
const String UNITS = "imperial";  // Options: metric, imperial
String cityName = "New York";   // Default city, configurable
String stateName = "NY";        // Default state, configurable

// Screen switching timing
const unsigned long SCREEN_SWITCH_INTERVAL = 30000;
unsigned long lastScreenChange = 0;
bool showTimeScreen = true;

// Weather update interval (configurable)
unsigned long WEATHER_UPDATE_INTERVAL = 5 * 60 * 1000; // Default: 5 minutes

// Time variables
int hours = 0;
int minutes = 0;
int seconds = 0;
int dayOfMonth = 1;
int month = 1;
int year = 2025;
String dayOfWeekStr = "MON";
String monthStr = "JAN";
int currentHour = 12;
bool timeInitialized = false;
unsigned long lastTimeUpdate = 0;
unsigned long lastSecondUpdate = 0;
float timezone = -5.0; // Default to Eastern Time (UTC-5)

// Weather variables
int currentTemp = 0;
int lowTemp = 0;
int highTemp = 0;
String currentCondition = "Unknown";
int humidity = 0;
int sunriseHour = 6;
int sunriseMinute = 0;
int sunsetHour = 18;
int sunsetMinute = 0;
unsigned long lastWeatherUpdate = 0;

// Weather forecast data
WeatherDay forecast[6] = {
  {"???", 0, 0, 0}, // Will be populated from API
  {"???", 0, 0, 0},
  {"???", 0, 0, 0},
  {"???", 0, 0, 0},
  {"???", 0, 0, 0},
  {"???", 0, 0, 0}
};

// Weather icon bitmaps
const uint8_t sunny_icon[8] = {
  0x10, 0x54, 0x38, 0xFE, 0x38, 0x54, 0x10, 0x00
};

const uint8_t partly_cloudy_icon[8] = {
  0x08, 0x54, 0x38, 0x44, 0x3E, 0x00, 0x00, 0x00
};

const uint8_t cloudy_icon[8] = {
  0x00, 0x00, 0x78, 0x84, 0xFE, 0x00, 0x00, 0x00
};

const uint8_t foggy_icon[8] = {
  0x00, 0xEE, 0x00, 0xFE, 0x00, 0x7C, 0x00, 0x00
};

const uint8_t rainy_icon[8] = {
  0x78, 0xFC, 0x00, 0x28, 0x28, 0x00, 0x00, 0x00
};

const uint8_t snowy_icon[8] = {
  0x78, 0xFC, 0x00, 0x10, 0x38, 0x10, 0x00, 0x00
};
