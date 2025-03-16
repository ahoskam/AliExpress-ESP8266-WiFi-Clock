/*
 * Display functions for ESP-01 Weather Display
 * Handles drawing on OLED display
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "config.h"

// Draw weather icon based on type
void drawWeatherIcon(int x, int y, byte iconType, byte size);

// Draw extra large weather icon (custom function for current weather)
void drawExtraLargeWeatherIcon(int x, int y, byte iconType);

// Draw the time screen with sun position indicator
void drawTimeScreen();

// Draw current weather screen
void drawCurrentWeatherScreen();

// Draw forecast screen
void drawForecastScreen();

#endif // DISPLAY_H
