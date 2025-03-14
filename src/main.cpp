/*
 * ESP-01 Weather & Time Display
 * Using Arduino Framework with U8g2 Library
 * For ESP-01 with 0.96" OLED display
 * Improved forecast grid spacing
 */

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// Define pins for I2C on ESP-01 (correct assignment)
#define SDA_PIN 0  // GPIO0
#define SCL_PIN 2  // GPIO2

// Initialize U8g2 display instance with software I2C
// Using U8G2_R2 to rotate display 180 degrees (fix upside-down issue)
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R2, SCL_PIN, SDA_PIN, U8X8_PIN_NONE);

// Screen switching interval (30 seconds)
const unsigned long SCREEN_SWITCH_INTERVAL = 30000;
unsigned long lastScreenChange = 0;
bool showTimeScreen = true;

// Time & Date variables - in real project, these would come from NTP
int hours = 10;
int minutes = 42;
String dayOfWeek = "WED";
String month = "MAR";
int dayOfMonth = 12;

// Weather variables - in real project, these would come from a weather API
int currentTemp = 24;
int lowTemp = 16;
String currentCondition = "SUNNY";

// Sun position variables
int sunriseHour = 6;
int sunsetHour = 18;
int currentHour = 10; // Would come from NTP in real implementation

// Weather forecast data structure
struct WeatherDay {
  String day;
  int temp;
  byte iconType; // 0=sunny, 1=partly cloudy, 2=cloudy, 3=foggy, 4=rainy, 5=snowy
};

// Weather forecast data - would come from API in real implementation
WeatherDay forecast[6] = {
  {"THU", 23, 0}, // Sunny
  {"FRI", 21, 1}, // Partly cloudy
  {"SAT", 19, 2}, // Cloudy
  {"SUN", 17, 3}, // Foggy
  {"MON", 20, 4}, // Rainy
  {"TUE", 22, 0}  // Sunny
};

// Custom weather icons as 8x8 bitmaps
static const uint8_t sunny_icon[] = {
  0x10, // 00010000
  0x54, // 01010100
  0x38, // 00111000
  0xFE, // 11111110
  0x38, // 00111000
  0x54, // 01010100
  0x10, // 00010000
  0x00  // 00000000
};

static const uint8_t partly_cloudy_icon[] = {
  0x08, // 00001000
  0x54, // 01010100
  0x38, // 00111000
  0x44, // 01000100
  0x3E, // 00111110
  0x00, // 00000000
  0x00, // 00000000
  0x00  // 00000000
};

static const uint8_t cloudy_icon[] = {
  0x00, // 00000000
  0x00, // 00000000
  0x78, // 01111000
  0x84, // 10000100
  0xFE, // 11111110
  0x00, // 00000000
  0x00, // 00000000
  0x00  // 00000000
};

static const uint8_t foggy_icon[] = {
  0x00, // 00000000
  0xEE, // 11101110
  0x00, // 00000000
  0xFE, // 11111110
  0x00, // 00000000
  0x7C, // 01111100
  0x00, // 00000000
  0x00  // 00000000
};

static const uint8_t rainy_icon[] = {
  0x78, // 01111000
  0xFC, // 11111100
  0x00, // 00000000
  0x28, // 00101000
  0x28, // 00101000
  0x00, // 00000000
  0x00, // 00000000
  0x00  // 00000000
};

static const uint8_t snowy_icon[] = {
  0x78, // 01111000
  0xFC, // 11111100
  0x00, // 00000000
  0x10, // 00010000
  0x38, // 00111000
  0x10, // 00010000
  0x00, // 00000000
  0x00  // 00000000
};

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
  
  // Format date
  char dateStr[12];
  sprintf(dateStr, "%s %s %d", dayOfWeek.c_str(), month.c_str(), dayOfMonth);
  
  // Draw time in large font
  u8g2.setFont(u8g2_font_inb19_mn);
  int timeWidth = u8g2.getStrWidth(timeStr);
  u8g2.drawStr(64 - timeWidth / 2, 30, timeStr);
  
  // Draw date in smaller font
  u8g2.setFont(u8g2_font_t0_11_tf);
  int dateWidth = u8g2.getStrWidth(dateStr);
  u8g2.drawStr(64 - dateWidth / 2, 45, dateStr);
  
  // Draw sun position bar
  int barStart = 24;
  int barEnd = 104;
  int barY = 55;
  
  // Draw bar line (dashed)
  for (int x = barStart; x <= barEnd; x += 4) {
    u8g2.drawPixel(x, barY);
    u8g2.drawPixel(x + 1, barY);
  }
  
  // Calculate sun position
  int dayMinutes = (currentHour - sunriseHour) * 60 + minutes;
  int totalDayMinutes = (sunsetHour - sunriseHour) * 60;
  float progress = constrain((float)dayMinutes / totalDayMinutes, 0.0, 1.0);
  int sunX = barStart + progress * (barEnd - barStart);
  
  // Draw sunrise icon (circle with rays)
  u8g2.drawCircle(barStart, barY, 2, U8G2_DRAW_ALL);
  u8g2.drawLine(barStart, barY - 3, barStart, barY - 5);
  u8g2.drawLine(barStart - 3, barY, barStart - 5, barY);
  u8g2.drawLine(barStart - 2, barY - 2, barStart - 3, barY - 3);
  
  // Draw sunset icon (circle with rays)
  u8g2.drawCircle(barEnd, barY, 2, U8G2_DRAW_ALL);
  u8g2.drawLine(barEnd, barY - 3, barEnd, barY - 5);
  u8g2.drawLine(barEnd + 3, barY, barEnd + 5, barY);
  u8g2.drawLine(barEnd + 2, barY - 2, barEnd + 3, barY - 3);
  
  // Draw sun position indicator (circle)
  u8g2.drawCircle(sunX, barY, 3, U8G2_DRAW_ALL);
}

// Draw the weather screen with forecast
void drawWeatherScreen() {
  // ===== IMPROVED LAYOUT =====
  // Reduce the size of the "today" section slightly
  // Move the divider left to give more space for the forecast grid
  
  // Draw "TODAY" label
  u8g2.setFont(u8g2_font_t0_11_tf);
  int todayWidth = u8g2.getStrWidth("TODAY");
  u8g2.drawStr(28 - todayWidth / 2, 10, "TODAY");  // Moved left from 32 to 28
  
  // Draw current weather icon
  byte currentIconType = 0; // Assuming sunny for current weather
  drawWeatherIcon(28, 28, currentIconType, 3);  // Moved left from 32 to 28
  
  // Draw current temperature
  char tempStr[5];
  sprintf(tempStr, "%d°", currentTemp);
  int tempWidth = u8g2.getStrWidth(tempStr);
  u8g2.drawUTF8(28 - tempWidth / 2, 50, tempStr);  // Moved left from 32 to 28
  
  // Draw vertical divider - moved left to give more space for forecast
  int dividerX = 58;  // Moved from 64 to 58
  for (int y = 8; y <= 56; y += 3) {
    u8g2.drawPixel(dividerX, y);
  }
  
  // ===== IMPROVED FORECAST GRID =====
  // Start the grid further left
  // Increase column width for better spacing between days
  
  // Draw 6-day forecast (2x3 grid)
  int startX = dividerX + 8;  // Start 8 pixels right of divider (was 70)
  int startY = 10;
  int colWidth = 22;  // Increased from 20 to 22
  int rowHeight = 27;
  
  for (int row = 0; row < 2; row++) {
    for (int col = 0; col < 3; col++) {
      int index = row * 3 + col;
      int x = startX + col * colWidth;
      int y = startY + row * rowHeight;
      
      // Draw day abbreviation
      u8g2.setFont(u8g2_font_t0_11_tf);
      u8g2.drawStr(x, y, forecast[index].day.c_str());
      
      // Draw weather icon
      drawWeatherIcon(x + 7, y + 10, forecast[index].iconType, 1);
      
      // Draw temperature
      char smallTempStr[5];
      sprintf(smallTempStr, "%d°", forecast[index].temp);
      int smallTempWidth = u8g2.getStrWidth(smallTempStr);
      u8g2.drawUTF8(x + 7 - smallTempWidth / 2, y + 20, smallTempStr);
    }
  }
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("\n\nESP-01 Weather & Time Display");
  
  // Initialize display
  u8g2.begin();
  Serial.println("Display initialized");
  
  // Initial display update
  lastScreenChange = millis();
  
  // In a real application, you would initialize WiFi here
  // and connect to a weather API and NTP server
}

void loop() {
  // Check if it's time to switch screens
  unsigned long currentMillis = millis();
  if (currentMillis - lastScreenChange >= SCREEN_SWITCH_INTERVAL) {
    showTimeScreen = !showTimeScreen;
    lastScreenChange = currentMillis;
    Serial.println(showTimeScreen ? "Switching to time screen" : "Switching to weather screen");
  }
  
  // Update display
  u8g2.clearBuffer();
  
  // Draw the appropriate screen
  if (showTimeScreen) {
    drawTimeScreen();
  } else {
    drawWeatherScreen();
  }
  
  // Send buffer to display
  u8g2.sendBuffer();
  
  // Short delay to prevent too frequent updates
  delay(100);
  
  // In a real application, you would periodically update the time and weather data
}