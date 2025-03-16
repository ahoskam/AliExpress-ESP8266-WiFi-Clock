/*
 * Implementation of Display functions
 */

#include "display.h"
#include "weather.h"
#include "time_manager.h"

// Draw weather icon based on type
void drawWeatherIcon(int x, int y, byte iconType, byte size) {
  const uint8_t* icon;
  
  // Select icon based on type
  switch(iconType) {
    case 0: icon = sunny_icon; break;
    case 1: icon = partly_cloudy_icon; break;
    case 2: icon = cloudy_icon; break;
    case 3: icon = foggy_icon; break;
    case 4: icon = rainy_icon; break;
    case 5: icon = snowy_icon; break;
    default: icon = sunny_icon;
  }
  
  // Draw the icon scaled to size
  if (size == 1) {
    // Small 8x8 icon
    u8g2.drawXBM(x - 4, y - 4, 8, 8, icon);
  } else if (size == 2) {
    // Medium 16x16 icon (scaled version of 8x8)
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        if (icon[i] & (1 << j)) {
          u8g2.drawBox(x - 8 + j*2, y - 8 + i*2, 2, 2);
        }
      }
    }
  } else {
    // Large icon (size 3 or more - scaled version of 8x8)
    int pixelSize = size - 2; // 1 for size 3, 2 for size 4, etc.
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        if (icon[i] & (1 << j)) {
          u8g2.drawBox(x - 12 + j*3, y - 12 + i*3, pixelSize + 2, pixelSize + 2);
        }
      }
    }
  }
}

// Draw extra large weather icon (custom function for current weather)
void drawExtraLargeWeatherIcon(int x, int y, byte iconType) {
  const uint8_t* icon;
  
  // Select icon based on type
  switch(iconType) {
    case 0: icon = sunny_icon; break;
    case 1: icon = partly_cloudy_icon; break;
    case 2: icon = cloudy_icon; break;
    case 3: icon = foggy_icon; break;
    case 4: icon = rainy_icon; break;
    case 5: icon = snowy_icon; break;
    default: icon = sunny_icon;
  }
  
  // Draw ultra-large icon (much bigger than regular scaling)
  int pixelSize = 4; // Each bit becomes a 4x4 pixel block
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (icon[i] & (1 << j)) {
        u8g2.drawBox(x - 16 + j*4, y - 16 + i*4, pixelSize, pixelSize);
      }
    }
  }
}

// Draw the time screen with sun position indicator
void drawTimeScreen() {
  // Format time with leading zero if needed
  char timeStr[10]; // Increased buffer size for 12-hour format with AM/PM
  formatTimeString(timeStr, hours, minutes, use12HourFormat);
  
  // Format date with buffer size control
  char dateStr[32]; // Increased buffer size to 32 to ensure sufficient space
  snprintf(dateStr, sizeof(dateStr), "%s %s %d", dayOfWeekStr.c_str(), monthStr.c_str(), dayOfMonth);
  
  // Draw time in large font
  u8g2.setFont(u8g2_font_logisoso24_tn);
  int timeWidth = u8g2.getStrWidth(timeStr);
  u8g2.drawStr(64 - timeWidth / 2, 32, timeStr);
  
  // Draw date in smaller font
  u8g2.setFont(u8g2_font_t0_11_tf);
  int dateWidth = u8g2.getStrWidth(dateStr);
  u8g2.drawStr(64 - dateWidth / 2, 48, dateStr);
  
  // Draw sun position bar (simplified version)
  int barStart = 24;
  int barEnd = 104;
  int barY = 55;
  
  // Draw bar line (dashed)
  for (int x = barStart; x <= barEnd; x += 4) {
    u8g2.drawPixel(x, barY);
    u8g2.drawPixel(x + 1, barY);
  }
  
  // Calculate minutes for sunrise, sunset, and current time
  int sunriseMinutes = sunriseHour * 60 + sunriseMinute;
  int sunsetMinutes = sunsetHour * 60 + sunsetMinute;
  int currentMinutes = currentHour * 60 + minutes;
  
  // Simple day/night detection
  bool isDaytime = false;
  
  // Handle case where day spans midnight
  if (sunsetMinutes < sunriseMinutes) {
    // If sunset is before sunrise, we're in a location where the day might span midnight
    // In this case, it's day if current time is after sunrise OR before sunset
    isDaytime = (currentMinutes >= sunriseMinutes) || (currentMinutes <= sunsetMinutes);
  } else {
    // Normal case: it's day if current time is between sunrise and sunset
    isDaytime = (currentMinutes >= sunriseMinutes) && (currentMinutes <= sunsetMinutes);
  }
  
  // Draw sunrise and sunset icons
  // Sunrise icon (circle with rays)
  u8g2.drawCircle(barStart, barY, 2, U8G2_DRAW_ALL);
  u8g2.drawLine(barStart, barY - 3, barStart, barY - 5);
  u8g2.drawLine(barStart - 3, barY, barStart - 5, barY);
  u8g2.drawLine(barStart - 2, barY - 2, barStart - 3, barY - 3);
  
  // Sunset icon (circle with rays)
  u8g2.drawCircle(barEnd, barY, 2, U8G2_DRAW_ALL);
  u8g2.drawLine(barEnd, barY - 3, barEnd, barY - 5);
  u8g2.drawLine(barEnd + 3, barY, barEnd + 5, barY);
  u8g2.drawLine(barEnd + 2, barY - 2, barEnd + 3, barY - 3);
  
  // Calculate sun position (simplified and more robust)
  int sunX = barStart;
  int barLength = barEnd - barStart;
  float progressPercent = 0.0;
  
  if (isDaytime) {
    // For daytime, calculate percentage through the day
    if (sunsetMinutes >= sunriseMinutes) {
      // Normal case
      int dayLength = sunsetMinutes - sunriseMinutes;
      int minutesSinceSunrise = currentMinutes - sunriseMinutes;
      
      // Ensure values are valid
      dayLength = max(1, dayLength);
      minutesSinceSunrise = max(0, min(minutesSinceSunrise, dayLength));
      
      progressPercent = (float)minutesSinceSunrise / dayLength;
    } else {
      // Day spans midnight (unusual case)
      int dayLength = (24 * 60) - sunriseMinutes + sunsetMinutes;
      int minutesSinceSunrise;
      
      if (currentMinutes >= sunriseMinutes) {
        minutesSinceSunrise = currentMinutes - sunriseMinutes;
      } else {
        minutesSinceSunrise = (24 * 60) - sunriseMinutes + currentMinutes;
      }
      
      // Ensure values are valid
      dayLength = max(1, dayLength);
      minutesSinceSunrise = max(0, min(minutesSinceSunrise, dayLength));
      
      progressPercent = (float)minutesSinceSunrise / dayLength;
    }
  } else {
    // For nighttime, calculate percentage through the night
    if (sunriseMinutes > sunsetMinutes) {
      // Normal night case (sunset today -> sunrise tomorrow)
      int nightLength = (24 * 60) - sunsetMinutes + sunriseMinutes;
      int minutesSinceSunset;
      
      if (currentMinutes >= sunsetMinutes) {
        // After sunset, before midnight
        minutesSinceSunset = currentMinutes - sunsetMinutes;
      } else {
        // After midnight, before sunrise
        minutesSinceSunset = (24 * 60) - sunsetMinutes + currentMinutes;
      }
      
      // Ensure values are valid
      nightLength = max(1, nightLength);
      minutesSinceSunset = max(0, min(minutesSinceSunset, nightLength));
      
      progressPercent = (float)minutesSinceSunset / nightLength;
    } else {
      // Unusual night case (sunset tomorrow -> sunrise today)
      int nightLength = sunriseMinutes - sunsetMinutes;
      int minutesSinceSunset = currentMinutes - sunsetMinutes;
      
      // Ensure values are valid
      nightLength = max(1, nightLength);
      minutesSinceSunset = max(0, min(minutesSinceSunset, nightLength));
      
      progressPercent = (float)minutesSinceSunset / nightLength;
    }
  }
  
  // Constrain progress to valid range
  progressPercent = constrain(progressPercent, 0.0, 1.0);
  
  // Calculate position based on percentage
  sunX = barStart + (int)(progressPercent * barLength);
  
  // Ensure sunX is within valid range
  sunX = constrain(sunX, barStart, barEnd);
  
  // Draw sun position indicator (enhanced)
  if (isDaytime) {
    // Daytime: filled circle for sun
    u8g2.drawDisc(sunX, barY, 3, U8G2_DRAW_ALL);
  } else {
    // Nighttime: moon shape (crescent)
    u8g2.drawCircle(sunX, barY, 3, U8G2_DRAW_ALL);
    u8g2.drawDisc(sunX + 1, barY, 2, U8G2_DRAW_ALL);
  }
}

// Draw the current weather screen
void drawCurrentWeatherScreen() {
  // Draw "TODAY" label at the top
  u8g2.setFont(u8g2_font_t0_11_tf);
  int todayWidth = u8g2.getStrWidth("TODAY");
  u8g2.drawStr(64 - todayWidth / 2, 10, "TODAY");
  
  // Draw current temperature in large font on the left
  u8g2.setFont(u8g2_font_logisoso24_tn);  // Larger font for temperature
  char tempStr[8];
  sprintf(tempStr, "%d", currentTemp);  // Just the number without degree
  int tempWidth = u8g2.getStrWidth(tempStr);
  
  // Position it on the left side
  int tempX = 25;
  u8g2.drawStr(tempX - tempWidth / 2, 36, tempStr);
  
  // Add a proper degree symbol manually (positioned much higher)
  u8g2.drawCircle(tempX + tempWidth / 2 + 3, 17, 2, U8G2_DRAW_ALL);
  
  // Draw current weather icon (extra large) on the right
  byte currentIconType = Weather::getWeatherIconType(currentCondition);
  drawExtraLargeWeatherIcon(96, 32, currentIconType);  // Using custom extra large function
  
  // Draw high/low temperatures at the bottom
  u8g2.setFont(u8g2_font_t0_11_tf);
  char highStr[10], lowStr[10];
  sprintf(highStr, "High: %d", highTemp);
  sprintf(lowStr, "Low: %d", lowTemp);
  
  // Draw circle for degree symbol rather than using UTF8
  int highWidth = u8g2.getStrWidth(highStr);
  int lowWidth = u8g2.getStrWidth(lowStr);
  
  u8g2.drawStr(32 - highWidth / 2, 55, highStr);
  u8g2.drawCircle(32 + highWidth / 2 + 2, 47, 1, U8G2_DRAW_ALL);  // Raised higher and moved right by 1 pixel
  
  u8g2.drawStr(96 - lowWidth / 2, 55, lowStr);
  u8g2.drawCircle(96 + lowWidth / 2 + 2, 47, 1, U8G2_DRAW_ALL);  // Raised higher and moved right by 1 pixel
}

// Draw the forecast screen
void drawForecastScreen() {
  // Draw title
  u8g2.setFont(u8g2_font_t0_11_tf);
  int forecastWidth = u8g2.getStrWidth("FORECAST");
  u8g2.drawStr(64 - forecastWidth / 2, 10, "FORECAST");
  
  // Draw 3-day forecast
  int startY = 18;
  int colWidth = 42;  // Divide screen into 3 equal columns
  
  for (int i = 0; i < 3; i++) {
    int x = i * colWidth + colWidth/2;  // Center of each column
    
    // Draw day name
    int dayWidth = u8g2.getStrWidth(forecast[i].day.c_str());
    u8g2.drawStr(x - dayWidth / 2, startY, forecast[i].day.c_str());
    
    // Draw weather icon
    drawWeatherIcon(x, startY + 12, forecast[i].iconType, 2);
    
    // Draw high temperature
    char highStr[10];
    sprintf(highStr, "H:%d", forecast[i].temp);
    int highWidth = u8g2.getStrWidth(highStr);
    u8g2.drawStr(x - highWidth / 2, startY + 25, highStr);
    // Draw degree symbol as a circle (positioned higher and spaced out more)
    u8g2.drawCircle(x + highWidth / 2 + 2, startY + 17, 1, U8G2_DRAW_ALL);
    
    // Draw low temperature (if available)
    char lowStr[10];
    if (forecast[i].lowTemp != -999) {
      sprintf(lowStr, "L:%d", forecast[i].lowTemp);
    } else {
      strcpy(lowStr, "L:--");
    }
    int lowWidth = u8g2.getStrWidth(lowStr);
    u8g2.drawStr(x - lowWidth / 2, startY + 35, lowStr);
    // Draw degree symbol as a circle (if we have a temp value, positioned higher and spaced out more)
    if (forecast[i].lowTemp != -999) {
      u8g2.drawCircle(x + lowWidth / 2 + 2, startY + 27, 1, U8G2_DRAW_ALL);
    }
  }
}