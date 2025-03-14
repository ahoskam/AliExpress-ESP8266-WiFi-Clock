/*
 * Weather functions for ESP-01 Weather Display
 * Handles OpenWeatherMap API calls and data processing
 */

#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>
#include "config.h"

// Fetch weather data from OpenWeatherMap API
bool fetchWeatherData();

// Get the appropriate icon type based on weather condition
byte getWeatherIconType(const String& condition);

#endif // WEATHER_H
