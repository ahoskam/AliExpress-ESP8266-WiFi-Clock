/*
 * Implementation of Time Manager functions
 */

#include "time_manager.h"
#include <time.h>

// Setup NTP client
void setupNTP() {
  // Configure time zone and DST settings
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  // Initialize NTP client
  timeClient.begin();
  timeClient.setTimeOffset(0); // Set to UTC, we'll handle timezone later
  
  Serial.println("NTP client initialized");
}

// Function to check if DST should be applied based on US rules
bool shouldApplyDST(const struct tm* timeinfo) {
    int month = timeinfo->tm_mon + 1;  // tm_mon is 0-based
    int day = timeinfo->tm_mday;
    
    // April through October are definitely DST
    if (month > 3 && month < 11) {
        return true;
    }
    // March: DST starts on the second Sunday
    else if (month == 3) {
        // Calculate the day of the second Sunday
        int weekday = timeinfo->tm_wday;
        int secondSunday = 8 + (7 - ((weekday + 8 - 1) % 7));
        return day >= secondSunday;
    }
    // November: DST ends on the first Sunday
    else if (month == 11) {
        // Calculate the day of the first Sunday
        int weekday = timeinfo->tm_wday;
        int firstSunday = 1 + (7 - ((weekday + 1 - 1) % 7));
        return day < firstSunday;
    }
    return false;
}

// Update time and date from NTP
bool updateTimeAndDate() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot update time - WiFi not connected");
    return false;
  }
  
  Serial.println("[Time] Attempting to update time from NTP server...");
  
  // Directly use configTime to get proper time 
  configTime(timezone * 3600, useDST ? 3600 : 0, "pool.ntp.org", "time.nist.gov");
  
  // Wait for time to be set
  time_t now = time(nullptr);
  int retry = 0;
  while (now < 24 * 3600 && retry < 10) {
    delay(500);
    now = time(nullptr);
    retry++;
  }
  
  if (now < 24 * 3600) {
    Serial.println("NTP time sync failed");
    return false;
  }
  
  // Get the time components directly
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return false;
  }
  
  // Update our time variables directly from the system time
  hours = timeinfo.tm_hour;
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;
  dayOfMonth = timeinfo.tm_mday;
  month = timeinfo.tm_mon + 1; // tm_mon is 0-based
  year = timeinfo.tm_year + 1900;
  
  // Get day of week (0 = Sunday, 1 = Monday, etc.)
  int dayOfWeek = timeinfo.tm_wday;
  
  // Convert to string representations
  dayOfWeekStr = getDayOfWeekShort(dayOfWeek);
  monthStr = getMonthShort(month);
  
  currentHour = hours;
  
  timeInitialized = true;
  lastSecondUpdate = millis();
  lastTimeUpdate = millis();
  
  // Log current time
  Serial.printf("[Time] Time set: %02d:%02d:%02d %s %s %d, %d\n", 
      hours, minutes, seconds, dayOfWeekStr.c_str(), monthStr.c_str(), dayOfMonth, year);
  Serial.printf("[Time] Timezone: UTC%s%g%s\n", 
      timezone >= 0 ? "+" : "", timezone, 
      useDST && shouldApplyDST(&timeinfo) ? " (DST active)" : "");
  
  return true;
}

// Update current time based on the last NTP sync (internal clock)
void updateCurrentTime() {
  if (!timeInitialized) return;
  
  // Calculate how many milliseconds have passed since last update
  unsigned long currentMillis = millis();
  unsigned long elapsedMs;
  
  // Handle millis() rollover
  if (currentMillis < lastSecondUpdate) {
    elapsedMs = (UINT_MAX - lastSecondUpdate) + currentMillis + 1;
    
    // Force time resync after rollover
    if (WiFi.status() == WL_CONNECTED) {
      updateTimeAndDate();
      return;
    }
  } else {
    elapsedMs = currentMillis - lastSecondUpdate;
  }
  
  // Only update if at least one second has passed
  if (elapsedMs >= 1000) {
    // Calculate number of seconds to add
    int secondsToAdd = elapsedMs / 1000;
    
    // Update the last second update time
    lastSecondUpdate = currentMillis - (elapsedMs % 1000);
    
    // Add seconds and handle minute/hour overflow
    seconds += secondsToAdd;
    
    while (seconds >= 60) {
      seconds -= 60;
      minutes++;
      
      // Log each minute change
      Serial.printf("[Time] Current time: %02d:%02d\n", hours, minutes);
      
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        
        Serial.printf("[Time] Hour changed to: %02d\n", hours);
        
        if (hours >= 24) {
          hours = 0;
          Serial.println("[Time] Day changed - triggering NTP resync");
          
          // At midnight, sync with NTP to handle date changes
          if (WiFi.status() == WL_CONNECTED) {
            updateTimeAndDate();
          }
        }
      }
    }
    
    // Update current hour for display
    currentHour = hours;
    
    // Force NTP resync every 30 minutes to avoid drift
    if (minutes % 30 == 0 && seconds < 10 && WiFi.status() == WL_CONNECTED) {
      Serial.println("[Time] Scheduled resync to prevent drift");
      updateTimeAndDate();
    }
  }
}

// Helper function to get day of week short name
String getDayOfWeekShort(int dayNum) {
  switch (dayNum) {
    case 0: return "SUN";
    case 1: return "MON";
    case 2: return "TUE";
    case 3: return "WED";
    case 4: return "THU";
    case 5: return "FRI";
    case 6: return "SAT";
    default: return "???";
  }
}

// Helper function to get month short name
String getMonthShort(int monthNum) {
  switch (monthNum) {
    case 1: return "JAN";
    case 2: return "FEB";
    case 3: return "MAR";
    case 4: return "APR";
    case 5: return "MAY";
    case 6: return "JUN";
    case 7: return "JUL";
    case 8: return "AUG";
    case 9: return "SEP";
    case 10: return "OCT";
    case 11: return "NOV";
    case 12: return "DEC";
    default: return "???";
  }
}

// Get current epoch time
time_t getEpochTime() {
  time_t now;
  time(&now);
  return now;
}

// Format time string in either 12-hour or 24-hour format
void formatTimeString(char* buffer, int hour, int minute, bool use12Hour) {
  if (use12Hour) {
    // 12-hour format with AM/PM
    int hour12 = hour % 12;
    if (hour12 == 0) hour12 = 12; // 0 hour should be displayed as 12 in 12-hour format
    const char* ampm = (hour < 12) ? "AM" : "PM";
    sprintf(buffer, "%2d:%02d %s", hour12, minute, ampm);
  } else {
    // 24-hour format
    sprintf(buffer, "%02d:%02d", hour, minute);
  }
}

// Reset time when timezone changes
void resetTimeWithNewTimezone() {
  // Reset the time initialization flag to force a full update
  timeInitialized = false;
  
  // Force an immediate update if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Resetting time with new timezone: " + String(timezone));
    
    // Reset the NTP client
    timeClient.end();
    timeClient.begin();
    timeClient.setTimeOffset(0); // We handle timezone manually
    
    // Force an update
    bool success = updateTimeAndDate();
    if (success) {
      Serial.println("Time reset successful with new timezone");
    } else {
      Serial.println("Time reset failed with new timezone");
    }
  } else {
    Serial.println("Cannot reset time - WiFi not connected");
  }
}

// Get local time with retry logic
bool getLocalTime(struct tm * info) {
  // Try up to 10 times to get a valid time
  for (int i = 0; i < 10; i++) {
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    if (timeinfo) {
      // Copy the data into the provided struct
      memcpy(info, timeinfo, sizeof(struct tm));
      return true;
    }
    delay(10);
  }
  
  return false;
}
