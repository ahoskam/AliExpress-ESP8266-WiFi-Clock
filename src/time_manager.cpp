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
void updateTimeAndDate() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot update time - WiFi not connected");
    return;
  }
  
  // Try to update time from NTP
  if (timeClient.update()) {
    // Get UTC time
    time_t utcTime = timeClient.getEpochTime();
    
    // Apply timezone offset (convert to seconds)
    time_t localTime = utcTime + (timezone * 3600);
    
    // Convert UTC time to tm structure for DST check
    struct tm *utcTm = gmtime(&utcTime);
    
    // Check if DST should be applied (for US timezones)
    bool shouldApplyDst = shouldApplyDST(utcTm);
    
    // Apply DST if needed (add 1 hour = 3600 seconds)
    if (shouldApplyDst && timezone < 0) {  // Only apply to US timezones (negative values)
      localTime += 3600;
      Serial.println("Applying DST adjustment (+1 hour)");
    }
    
    // Debug timezone conversion
    Serial.print("UTC time: ");
    Serial.print(utcTime);
    Serial.print(" -> Local time with timezone ");
    Serial.print(timezone);
    if (shouldApplyDst) Serial.print(" (with DST)");
    Serial.print(": ");
    Serial.println(localTime);
    
    // Convert to local time
    struct tm * ti;
    ti = gmtime(&localTime); // Use gmtime since we've already applied the offset
    
    // Update all time variables
    hours = ti->tm_hour;
    minutes = ti->tm_min;
    seconds = ti->tm_sec;
    dayOfMonth = ti->tm_mday;
    month = ti->tm_mon + 1; // tm_mon is 0-based
    year = ti->tm_year + 1900;
    
    // Get day of week (0 = Sunday, 1 = Monday, etc.)
    int dayOfWeek = ti->tm_wday;
    
    // Convert to string representations
    dayOfWeekStr = getDayOfWeekShort(dayOfWeek);
    monthStr = getMonthShort(month);
    
    currentHour = hours;
    
    timeInitialized = true;
    lastSecondUpdate = millis();
    lastTimeUpdate = millis();
    
    // Print time information using separate print statements
    Serial.print("Time updated: ");
    Serial.print(hours);
    Serial.print(":");
    Serial.print(minutes);
    Serial.print(":");
    Serial.print(seconds);
    Serial.print(" ");
    Serial.print(dayOfWeekStr);
    Serial.print(" ");
    Serial.print(monthStr);
    Serial.print(" ");
    Serial.print(dayOfMonth);
    Serial.print(", ");
    Serial.println(year);
    
    // Print timezone information using separate print statements
    Serial.print("Timezone offset: UTC");
    if (timezone >= 0) {
      Serial.print("+");
    }
    Serial.println(timezone);
  } else {
    Serial.println("Failed to update time");
  }
}

// Update current time based on the last NTP sync (internal clock)
void updateCurrentTime() {
  if (!timeInitialized) return;
  
  // Only update every second
  if (millis() - lastSecondUpdate < 1000) return;
  
  // Increment seconds
  seconds++;
  lastSecondUpdate = millis();
  
  // Update minutes and hours as needed
  if (seconds >= 60) {
    seconds = 0;
    minutes++;
    
    if (minutes >= 60) {
      minutes = 0;
      hours++;
      
      // Update currentHour when hours change
      currentHour = hours;
      
      if (hours >= 24) {
        hours = 0;
        // At midnight, we should sync with NTP again to handle date changes
        // This is a simplification that doesn't handle date rollover correctly
        updateTimeAndDate();
      }
      
      // Log time at the top of each hour
      Serial.print("Hour changed - Current time: ");
      Serial.print(hours);
      Serial.print(":");
      Serial.print(minutes);
      Serial.print(":");
      Serial.println(seconds);
    }
  }
  
  // Debug time info every 5 minutes
  static int lastMinuteLogged = -1;
  if (minutes % 5 == 0 && minutes != lastMinuteLogged) {
    Serial.print("Current time: ");
    Serial.print(hours);
    Serial.print(":");
    Serial.print(minutes);
    Serial.print(":");
    Serial.println(seconds);
    
    Serial.print("Current hour: ");
    Serial.println(currentHour);
    
    lastMinuteLogged = minutes;
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
    updateTimeAndDate();
  } else {
    Serial.println("Cannot reset time - WiFi not connected");
  }
}
