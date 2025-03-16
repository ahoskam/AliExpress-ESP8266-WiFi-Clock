/*
 * Time Manager functions for ESP-01 Weather Display
 * Handles NTP synchronization and time tracking
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include "config.h"

// Initialize NTP client
void setupNTP();

// Update time from NTP server
bool updateTimeAndDate();

// Update internal clock between NTP syncs
void updateCurrentTime();

// Reset time when timezone changes
void resetTimeWithNewTimezone();

// Helper functions for time display
String getDayOfWeekShort(int dayNum);
String getMonthShort(int monthNum);
time_t getEpochTime();

// Function to check if DST should be applied
bool shouldApplyDST(const struct tm* timeinfo);

// Function to format time in 12-hour format with AM/PM
void formatTimeString(char* buffer, int hour, int minute, bool use12Hour);

#endif // TIME_MANAGER_H
