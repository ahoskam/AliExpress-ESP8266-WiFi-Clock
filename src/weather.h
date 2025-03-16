/*
 * Weather functions for ESP-01 Weather Display
 * Handles OpenWeatherMap API calls and data processing
 */

#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "config.h"

namespace Weather {
    // Function declarations
    bool fetchWeatherData(void);
    byte getWeatherIconType(const String& condition);
}

#endif // WEATHER_H
