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
  
  // Try to update time from NTP with precise timing
  if (timeClient.update()) {
    // Get UTC time
    time_t utcTime = timeClient.getEpochTime();
    int currentSecond = timeClient.getSeconds();
    
    Serial.print("[Time] NTP Response - Epoch: ");
    Serial.print(utcTime);
    Serial.print(", Current second: ");
    Serial.println(currentSecond);
    
    // Implement precise synchronization
    // Wait until the start of the next second
    bool performPreciseSync = true;
    int startSecond = currentSecond;
    
    // Only do this if we're not too close to a second boundary already
    if (currentSecond < 58) { // Don't try if we're close to a minute boundary
      Serial.println("[Time] Performing precise synchronization to second boundary");
      
      // Wait for the next second to start
      while (timeClient.getSeconds() == startSecond) {
        delay(10);
      }
      
      // Now we're at the exact start of a new second
      Serial.println("[Time] Precise synchronization complete");
      lastSecondUpdate = millis(); // Update timestamp precisely at second boundary
      
      // Get new time after sync
      utcTime = timeClient.getEpochTime();
    } else {
      performPreciseSync = false;
      Serial.println("[Time] Skipping precise sync (too close to minute boundary)");
    }
    
    // If we have a previous sync, calculate drift
    if (lastNtpTimestamp > 0 && lastNtpMillis > 0) {
      unsigned long expectedElapsedSeconds = utcTime - lastNtpTimestamp;
      unsigned long actualElapsedMillis = millis() - lastNtpMillis;
      
      // Only calculate drift if reasonable time has passed (at least 5 minutes)
      if (expectedElapsedSeconds > 300) {
        // Convert expected seconds to milliseconds
        unsigned long expectedElapsedMillis = expectedElapsedSeconds * 1000;
        
        // Calculate drift in milliseconds
        long currentDrift = actualElapsedMillis - expectedElapsedMillis;
        
        // Convert to hourly drift rate (milliseconds per hour)
        // 3600 seconds in an hour
        float hoursElapsed = expectedElapsedSeconds / 3600.0;
        if (hoursElapsed > 0) {
          long driftPerHour = (long)(currentDrift / hoursElapsed);
          
          // Update the drift correction value with some smoothing
          // Give more weight to the new measurement (75%) and less to the old one (25%)
          driftCorrection = (driftPerHour * 3 + driftCorrection) / 4;
          
          Serial.print("[Time] Calculated drift: ");
          Serial.print(currentDrift);
          Serial.print(" ms over ");
          Serial.print(hoursElapsed);
          Serial.print(" hours (");
          Serial.print(driftPerHour);
          Serial.println(" ms/hour)");
          
          Serial.print("[Time] Updated drift correction: ");
          Serial.print(driftCorrection);
          Serial.println(" ms/hour");
        }
      }
    }
    
    // Save current timestamp and millis for future drift calculation
    lastNtpTimestamp = utcTime;
    lastNtpMillis = millis();
    
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
    
    Serial.print("UTC time: ");
    Serial.print(utcTime);
    Serial.print(" -> Local time with timezone ");
    Serial.print(timezone);
    Serial.print(" (");
    Serial.print(shouldApplyDst ? "with" : "without");
    Serial.print(" DST): ");
    Serial.println(localTime);
    
    // Convert to local time
    struct tm * ti;
    ti = gmtime(&localTime); // Use gmtime since we've already applied the offset
    
    // Update all time variables
    hours = ti->tm_hour;
    minutes = ti->tm_min;
    seconds = ti->tm_sec;
    
    // If we did precise sync, use the second value we know is accurate
    if (performPreciseSync) {
      seconds = timeClient.getSeconds();
    }
    
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
    // lastSecondUpdate is already set if we did precise sync
    if (!performPreciseSync) {
      lastSecondUpdate = millis();
    }
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
    
    return true;
  } else {
    Serial.println("Failed to update time");
    return false;
  }
}

// Update current time based on the last NTP sync (internal clock)
void updateCurrentTime() {
  if (!timeInitialized) return;
  
  // Calculate how many seconds have passed, with drift correction
  unsigned long currentMillis = millis();
  unsigned long elapsedMs;
  
  // Handle millis() rollover (occurs approximately every 49.7 days)
  if (currentMillis < lastSecondUpdate) {
    // Rollover occurred
    Serial.println("[Time] Detected millis() rollover");
    // UINT_MAX is the maximum value for unsigned long (4,294,967,295)
    elapsedMs = (UINT_MAX - lastSecondUpdate) + currentMillis + 1;
    
    // Also reset the lastNtpMillis to avoid drift calculation errors
    if (lastNtpMillis > currentMillis) {
      // Force an NTP update soon to resync
      lastTimeUpdate = 0;
    }
  } else {
    elapsedMs = currentMillis - lastSecondUpdate;
  }
  
  // Calculate hours since last NTP sync
  float hoursSinceSync;
  
  // Handle rollover for this calculation as well
  if (currentMillis < lastNtpMillis) {
    unsigned long millisSinceSync = (UINT_MAX - lastNtpMillis) + currentMillis + 1;
    hoursSinceSync = millisSinceSync / 3600000.0;
  } else {
    hoursSinceSync = (currentMillis - lastNtpMillis) / 3600000.0;
  }
  
  // Adjust elapsed time by removing the calculated drift
  // If drift correction is positive (clock runs fast), we decrease the elapsed time
  // If drift correction is negative (clock runs slow), we increase the elapsed time
  long correctedElapsedMs = elapsedMs;
  
  // Only apply correction if we've had at least one successful NTP sync
  if (lastNtpTimestamp > 0) {
    // Apply the drift correction relative to the last second update
    // We adjust the elapsed time since last second update, not the total drift since NTP sync
    float hoursSinceLastSecond = elapsedMs / 3600000.0;
    long correctionSinceLastSecond = (long)(driftCorrection * hoursSinceLastSecond);
    correctedElapsedMs = elapsedMs - correctionSinceLastSecond;
    
    // Ensure we don't go backward in time due to correction
    if (correctedElapsedMs < 0) correctedElapsedMs = 0;
  }
  
  // Only update if at least one second has passed
  if (correctedElapsedMs >= 1000) {
    // Calculate number of seconds that have elapsed
    int secondsToAdd = correctedElapsedMs / 1000;
    
    // Update the last second update time more precisely
    lastSecondUpdate = currentMillis - (correctedElapsedMs % 1000);
    
    // Add the elapsed seconds
    seconds += secondsToAdd;
    
    // Update minutes and hours as needed
    while (seconds >= 60) {
      seconds -= 60;
      minutes++;
      
      // Log the time every minute
      Serial.print("[Time] Current time: ");
      Serial.print(hours);
      Serial.print(":");
      if (minutes < 10) Serial.print("0");
      Serial.println(minutes);
      
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        
        // Log the hour change
        Serial.print("[Time] Hour changed to: ");
        Serial.println(hours);
        
        if (hours >= 24) {
          hours = 0;
          // Log the day change
          Serial.println("[Time] Day changed");
          
          // At midnight, we should sync with NTP again to handle date changes
          bool success = updateTimeAndDate();
          if (!success) {
            Serial.println("[Time] Failed to update time at day change");
          }
        }
      }
    }
    
    // Update current hour for display
    currentHour = hours;
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
