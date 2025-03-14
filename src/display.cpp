/*
 * Implementation of Display functions
 */

#include "display.h"
#include "weather.h"

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
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        if (icon[i] & (1 << j)) {
          u8g2.drawBox(x - 12 + j*3, y - 12 + i*3, 3, 3);
        }
      }
    }
  }
}

// Draw the time screen with sun position indicator
void drawTimeScreen() {
  // Format time with leading zero if needed
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", hours, minutes);
  
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
      minutesSinceSunrise = max(0, minutesSinceSunrise);
      
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
      minutesSinceSunrise = max(0, minutesSinceSunrise);
      
      progressPercent = (float)minutesSinceSunrise / dayLength;
    }
  } else {
    // For nighttime, calculate percentage through the night
    if (sunriseMinutes > sunsetMinutes) {
      // Normal night case
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
      minutesSinceSunset = max(0, minutesSinceSunset);
      
      progressPercent = (float)minutesSinceSunset / nightLength;
    } else {
      // Unusual night case
      int nightLength = sunriseMinutes - sunsetMinutes;
      int minutesSinceSunset = currentMinutes - sunsetMinutes;
      
      // Ensure values are valid
      nightLength = max(1, nightLength);
      minutesSinceSunset = max(0, minutesSinceSunset);
      
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
    u8g2.drawDisc(sunX + 1, barY - 1, 2, U8G2_DRAW_ALL);
  }
}

// Draw the weather screen with forecast
void drawWeatherScreen() {
  // Draw "TODAY" label
  u8g2.setFont(u8g2_font_t0_11_tf);
  int todayWidth = u8g2.getStrWidth("TODAY");
  u8g2.drawStr(21 - todayWidth / 2, 10, "TODAY");  // Centered at x=21
  
  // Draw current weather icon
  byte currentIconType = getWeatherIconType(currentCondition);
  drawWeatherIcon(21, 28, currentIconType, 3);  // Icon centered at x=21
  
  // Draw current temperature
  char tempStr[10];
  sprintf(tempStr, "%d°", currentTemp);
  int tempWidth = u8g2.getStrWidth(tempStr);
  u8g2.drawUTF8(21 - tempWidth / 2, 50, tempStr);  // Ensure it's centered at x=21
  
  // Draw vertical divider - moved further left
  int dividerX = 43;  // Moved further left from 45 to 43
  for (int y = 8; y <= 56; y += 3) {
    u8g2.drawPixel(dividerX, y);
  }
  
  // Draw forecast grid with more space
  int startX = 45 + 6;  // Keep the forecast grid at the same position
  int startY = 10;
  int colWidth = 25;  // Slightly increased column width to fill the extra space
  int rowHeight = 27;
  
  // The forecast array contains 6 days (indices 0-5)
  // Index 0 = tomorrow, Index 1 = day after tomorrow, etc.
  // We display these in a 2x3 grid (2 rows, 3 columns)
  // First row: days 1-3 (indices 0-2)
  // Second row: days 4-6 (indices 3-5)
  for (int row = 0; row < 2; row++) {
    for (int col = 0; col < 3; col++) {
      int index = row * 3 + col;  // Convert row/col to array index (0-5)
      int x = startX + col * colWidth;
      int y = startY + row * rowHeight;
      
      // Draw day abbreviation
      u8g2.setFont(u8g2_font_t0_11_tf);
      int dayWidth = u8g2.getStrWidth(forecast[index].day.c_str());
      u8g2.drawStr(x + 7 - dayWidth / 2, y, forecast[index].day.c_str());  // Center day text
      
      // Draw weather icon
      drawWeatherIcon(x + 7, y + 10, forecast[index].iconType, 1);
      
      // Draw high temperature only
      char smallTempStr[10];
      sprintf(smallTempStr, "%d°", forecast[index].temp);
      
      // Draw temperature
      int smallTempWidth = u8g2.getStrWidth(smallTempStr);
      u8g2.drawUTF8(x + 7 - smallTempWidth / 2, y + 20, smallTempStr);  // Already centered
    }
  }
}