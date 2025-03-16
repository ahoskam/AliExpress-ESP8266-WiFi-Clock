/*
 * Implementation of Weather functions
 */

#include "weather.h"
#include "time_manager.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <algorithm> // For std::min

// Define the size for JSON documents - reduce these values to handle memory constraints
// For ESP8266, we need to be careful with memory usage
#define CURRENT_WEATHER_JSON_SIZE 1024
#define FORECAST_JSON_SIZE 1024

namespace Weather {
    // Function prototypes
    static void trimString(String &str);
    static bool isValidCityName(const String &city);

    // Helper function to get weather icon type based on condition string
    byte getWeatherIconType(const String& condition) {
        if (condition.equalsIgnoreCase("Clear")) {
            return 0; // Sunny
        } else if (condition.equalsIgnoreCase("Clouds") || condition.indexOf("few") >= 0 || condition.indexOf("scattered") >= 0) {
            return 1; // Partly cloudy
        } else if (condition.indexOf("broken") >= 0 || condition.indexOf("overcast") >= 0) {
            return 2; // Cloudy
        } else if (condition.equalsIgnoreCase("Mist") || condition.equalsIgnoreCase("Fog") || condition.equalsIgnoreCase("Haze")) {
            return 3; // Foggy
        } else if (condition.equalsIgnoreCase("Rain") || condition.equalsIgnoreCase("Drizzle") || condition.indexOf("shower") >= 0) {
            return 4; // Rainy
        } else if (condition.equalsIgnoreCase("Snow") || condition.indexOf("snow") >= 0) {
            return 5; // Snowy
        } else {
            return 0; // Default to sunny
        }
    }

    // Helper function to trim whitespace from beginning and end of a string
    static void trimString(String &str) {
        while (str.length() > 0 && isSpace(str.charAt(0))) {
            str.remove(0, 1);
        }
        while (str.length() > 0 && isSpace(str.charAt(str.length() - 1))) {
            str.remove(str.length() - 1);
        }
    }

    // Helper function to check if city name is valid
    static bool isValidCityName(const String &city) {
        if (city.length() == 0 || city == "_") {
            return false;
        }
        
        // Check if city contains at least one alphanumeric character
        for (size_t i = 0; i < city.length(); i++) {
            if (isAlphaNumeric(city.charAt(i))) {
                return true;
            }
        }
        
        return false;
    }

    // Fetch weather data from OpenWeatherMap
    bool fetchWeatherData() {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Cannot fetch weather - WiFi not connected");
            return false;
        }
        
        // Check if API key is available
        if (API_KEY.length() < 5) {
            Serial.println("ERROR: No valid OpenWeatherMap API key found!");
            
            // Display error message on screen
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_6x10_tf);
            u8g2.drawStr(0, 10, "Missing API Key!");
            u8g2.drawStr(0, 25, "Please set your own");
            u8g2.drawStr(0, 40, "OpenWeatherMap API key");
            u8g2.drawStr(0, 55, "in settings page");
            u8g2.sendBuffer();
            delay(3000);
            return false;
        }
        
        // Trim whitespace from city and state names
        trimString(cityName);
        trimString(stateName);
        
        // Check if city name is valid
        if (!isValidCityName(cityName)) {
            Serial.println("ERROR: City name is empty or invalid, setting to default 'New York'");
            cityName = "New York";
            
            // Display error message on screen
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_6x10_tf);
            u8g2.drawStr(0, 10, "Invalid city name!");
            u8g2.drawStr(0, 25, "Please update settings");
            u8g2.drawStr(0, 40, "at config portal");
            u8g2.drawStr(0, 55, "Using: New York");
            u8g2.sendBuffer();
            delay(3000);
        }
        
        Serial.println("Original city name: '" + cityName + "', length: " + String(cityName.length()));
        Serial.println("Original state name: '" + stateName + "', length: " + String(stateName.length()));
        
        // Sanitize city and state names
        String cleanCity = cityName;
        String cleanState = stateName;
        
        // Remove any non-printable or problematic characters
        for (size_t i = 0; i < cleanCity.length(); i++) {
            if (!isprint(cleanCity[i]) || cleanCity[i] == ',' || cleanCity[i] == '&') {
                cleanCity.setCharAt(i, '_');
            }
        }
        
        for (size_t i = 0; i < cleanState.length(); i++) {
            if (!isprint(cleanState[i]) || cleanState[i] == ',' || cleanState[i] == '&') {
                cleanState.setCharAt(i, '_');
            }
        }
        
        // If city is just underscores or empty after cleaning, use default
        bool hasValidChar = false;
        for (size_t i = 0; i < cleanCity.length(); i++) {
            if (cleanCity[i] != '_') {
                hasValidChar = true;
                break;
            }
        }
        
        if (!hasValidChar || cleanCity.length() == 0) {
            Serial.println("WARNING: City name contains only underscores or is empty after sanitizing");
            Serial.println("Using default city: New York");
            cleanCity = "New York";
            cityName = "New York"; // Update the global variable too
        }
        
        WiFiClient client;
        HTTPClient http;
        
        // URL encode the city and state names to handle spaces and special characters
        String encodedCity = "";
        String encodedState = "";
        
        // Simple URL encoding for spaces and special characters
        for (size_t i = 0; i < cleanCity.length(); i++) {
            if (cleanCity[i] == ' ') {
                encodedCity += "%20";
            } else if (isAlphaNumeric(cleanCity[i]) || cleanCity[i] == '_' || cleanCity[i] == '.') {
                encodedCity += cleanCity[i];
            } else {
                // Convert other special characters to percent encoding
                char hex[4];
                sprintf(hex, "%%%02X", (unsigned char)cleanCity[i]);
                encodedCity += hex;
            }
        }
        
        for (size_t i = 0; i < cleanState.length(); i++) {
            if (cleanState[i] == ' ') {
                encodedState += "%20";
            } else if (isAlphaNumeric(cleanState[i]) || cleanState[i] == '_' || cleanState[i] == '.') {
                encodedState += cleanState[i];
            } else {
                // Convert other special characters to percent encoding
                char hex[4];
                sprintf(hex, "%%%02X", (unsigned char)cleanState[i]);
                encodedState += hex;
            }
        }
        
        // Current weather endpoint - using city and state with proper URL encoding
        String currentUrl = "http://api.openweathermap.org/data/2.5/weather?q=" + 
                           encodedCity + "," + encodedState + ",US&units=" + UNITS + "&appid=" + API_KEY;
        
        Serial.println("Fetching current weather...");
        Serial.print("City: '");
        Serial.print(cleanCity);
        Serial.println("'");
        Serial.print("State: '");
        Serial.print(cleanState);
        Serial.println("'");
        Serial.print("Encoded City: '");
        Serial.print(encodedCity);
        Serial.println("'");
        Serial.println("URL: " + currentUrl);
        
        http.begin(client, currentUrl);
        int httpCode = http.GET();
        
        Serial.print("HTTP Result Code: ");
        Serial.println(httpCode);
        
        if (httpCode == 200) {
            String payload = http.getString();
            Serial.println("Current weather received");
            
            // Log the full response for debugging
            Serial.println("Full current weather response:");
            Serial.println(payload);
            
            // Parse JSON using ArduinoJson with specified capacity
            StaticJsonDocument<CURRENT_WEATHER_JSON_SIZE> doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (!error) {
                // Parse current weather data
                currentTemp = doc["main"]["temp"];
                highTemp = doc["main"]["temp_max"];
                lowTemp = doc["main"]["temp_min"];
                humidity = doc["main"]["humidity"];
                
                // More detailed error handling for condition
                if (doc["weather"].is<JsonArray>() && doc["weather"].size() > 0) {
                    const char* condition = doc["weather"][0]["main"];
                    currentCondition = String(condition);
                } else {
                    Serial.println("Warning: Could not find weather condition in response");
                    currentCondition = "Unknown";
                }
                
                // Get sunrise and sunset times
                if (doc["sys"]["sunrise"].is<long>() && doc["sys"]["sunset"].is<long>()) {
                    long sunriseTimestamp = doc["sys"]["sunrise"];
                    long sunsetTimestamp = doc["sys"]["sunset"];
                    
                    // Convert to hours and minutes with timezone adjustment
                    // The timestamps from API are in UTC, so we need to apply the timezone offset
                    time_t sunriseTime = sunriseTimestamp + (timezone * 3600); // Apply timezone offset
                    time_t sunsetTime = sunsetTimestamp + (timezone * 3600);   // Apply timezone offset
                    
                    // Check if we should apply DST (US rules)
                    struct tm* stm = gmtime(&sunriseTime);
                    bool shouldApplyDst = shouldApplyDST(stm);
                        
                    if (shouldApplyDst && timezone < 0) {  // Only apply to US timezones
                        sunriseTime += 3600;  // Add an hour for DST
                        sunsetTime += 3600;   // Add an hour for DST
                        Serial.println("Applying DST adjustment (+1 hour) to sunrise/sunset times");
                    }
                    
                    // Convert to hours and minutes
                    sunriseHour = (sunriseTime / 3600) % 24;
                    sunriseMinute = (sunriseTime / 60) % 60;
                    sunsetHour = (sunsetTime / 3600) % 24;
                    sunsetMinute = (sunsetTime / 60) % 60;
                    
                    Serial.println("Sunrise (UTC): " + String(sunriseTimestamp) + 
                                 " -> Local: " + String(sunriseHour) + ":" + String(sunriseMinute));
                    Serial.println("Sunset (UTC): " + String(sunsetTimestamp) + 
                                 " -> Local: " + String(sunsetHour) + ":" + String(sunsetMinute));
                } else {
                    Serial.println("Warning: Could not find sunrise/sunset times");
                    // Use defaults
                    sunriseHour = 6;
                    sunriseMinute = 0;
                    sunsetHour = 18;
                    sunsetMinute = 0;
                }
                
                Serial.println("Current temp: " + String(currentTemp) + "°F");
                Serial.println("Condition: " + currentCondition);
                Serial.println("Sunrise: " + String(sunriseHour) + ":" + String(sunriseMinute));
                Serial.println("Sunset: " + String(sunsetHour) + ":" + String(sunsetMinute));
                
                http.end();
                
                // Now fetch forecast data using a more memory-efficient approach
                // Based on your previous working code
                String forecastUrl = "http://api.openweathermap.org/data/2.5/forecast?q=" + 
                                     encodedCity + "," + encodedState + ",US&units=" + UNITS + "&appid=" + API_KEY;
                
                Serial.println("Fetching forecast...");
                http.begin(client, forecastUrl);
                httpCode = http.GET();
                
                // Log the forecast URL and HTTP code
                Serial.println("Forecast URL: " + forecastUrl);
                Serial.print("Forecast HTTP Result Code: ");
                Serial.println(httpCode);
                
                if (httpCode == 200) {
                    // Get current day info with proper timezone adjustment
                    time_t now = time(nullptr);
                    time_t localNow = now + (timezone * 3600); // Apply timezone offset
                    
                    // Debug stream availability before we start
                    WiFiClient* stream = http.getStreamPtr();
                    Serial.print("Stream available bytes: ");
                    Serial.println(stream->available());
                    
                    // Create a document for parsing with appropriate size
                    StaticJsonDocument<FORECAST_JSON_SIZE> doc;
                    
                    // Use the JSON parser in stream mode
                    DeserializationError error = deserializeJson(doc, *stream);
                    
                    if (error) {
                        Serial.print("deserializeJson() failed: ");
                        Serial.println(error.c_str());
                        http.end();
                        return false;
                    }
                    
                    Serial.println("JSON parsed successfully");
                    JsonArray list = doc["list"];
                    Serial.print("Number of entries in list: ");
                    Serial.println(list.size());
                    
                    // Get current day info for date calculations
                    struct tm* currentTime = gmtime(&localNow);
                    int todayDate = currentTime->tm_mday;
                    int todayMonth = currentTime->tm_mon;
                    int todayYear = currentTime->tm_year;
                    int todayDayOfWeek = currentTime->tm_wday;
                    
                    // Debug current date
                    Serial.print("Today's date: ");
                    Serial.print(todayDate);
                    Serial.print("/");
                    Serial.print(todayMonth + 1);
                    Serial.print("/");
                    Serial.print(todayYear + 1900);
                    Serial.print(", Day of week: ");
                    Serial.println(getDayOfWeekShort(todayDayOfWeek));
                    
                    // Initialize forecast array
                    for (int i = 0; i < 5; i++) {
                        int futureDayOfWeek = (todayDayOfWeek + (i + 1)) % 7;
                        forecast[i].day = getDayOfWeekShort(futureDayOfWeek);
                        forecast[i].temp = -999; // Initialize to NA
                        forecast[i].lowTemp = -999;
                        forecast[i].iconType = 0;
                        
                        Serial.print("Pre-initialized forecast day ");
                        Serial.print(i);
                        Serial.print(": ");
                        Serial.println(forecast[i].day);
                    }
                    
                    // Track highest and lowest temperature for each day
                    float maxTempForDay[5] = {-999, -999, -999, -999, -999};
                    float minTempForDay[5] = {999, 999, 999, 999, 999};
                    String conditionForDay[5];
                    
                    // Process each forecast entry
                    for (JsonVariant entry : list) {
                        // Get timestamp and convert to local time
                        long timestamp = entry["dt"].as<long>();
                        time_t forecastTime = timestamp + (timezone * 3600);
                        
                        // Check if we should apply DST (US rules)
                        struct tm* ftm = gmtime(&forecastTime);
                        bool shouldApplyDst = shouldApplyDST(ftm);
                            
                        if (shouldApplyDst && timezone < 0) {  // Only apply to US timezones
                            forecastTime += 3600;  // Add an hour for DST
                        }
                        
                        struct tm* forecastTm = gmtime(&forecastTime);
                        
                        // Calculate days from today using proper date comparison
                        int daysFromToday;
                        if (forecastTm->tm_year == todayYear) {
                            if (forecastTm->tm_mon == todayMonth) {
                                daysFromToday = forecastTm->tm_mday - todayDate;
                            } else {
                                // Different month, same year
                                int daysInMonth;
                                switch (todayMonth) {
                                    case 1: daysInMonth = 31; break;  // January
                                    case 2: daysInMonth = ((todayYear % 4 == 0 && todayYear % 100 != 0) || todayYear % 400 == 0) ? 29 : 28; break;  // February
                                    case 3: daysInMonth = 31; break;  // March
                                    case 4: daysInMonth = 30; break;  // April
                                    case 5: daysInMonth = 31; break;  // May
                                    case 6: daysInMonth = 30; break;  // June
                                    case 7: daysInMonth = 31; break;  // July
                                    case 8: daysInMonth = 31; break;  // August
                                    case 9: daysInMonth = 30; break;  // September
                                    case 10: daysInMonth = 31; break; // October
                                    case 11: daysInMonth = 30; break; // November
                                    case 12: daysInMonth = 31; break; // December
                                    default: daysInMonth = 30; break;
                                }
                                daysFromToday = (daysInMonth - todayDate) + forecastTm->tm_mday;
                            }
                        } else {
                            // Different year
                            daysFromToday = 31 - todayDate + forecastTm->tm_mday;  // Simplified calculation for year boundary
                        }
                        
                        if (daysFromToday <= 0) continue; // Skip entries for today
                        if (daysFromToday > 6) continue;  // Skip entries too far in the future
                        
                        int forecastIndex = daysFromToday - 1;
                        
                        // Get temperature
                        float temp = entry["main"]["temp"].as<float>();
                        
                        // Update if this is the highest temperature we've seen for this day
                        if (temp > maxTempForDay[forecastIndex]) {
                            maxTempForDay[forecastIndex] = temp;
                            if (entry["weather"][0]["main"]) {
                                conditionForDay[forecastIndex] = entry["weather"][0]["main"].as<const char*>();
                            }
                        }
                        
                        // Update if this is the lowest temperature we've seen for this day
                        if (temp < minTempForDay[forecastIndex]) {
                            minTempForDay[forecastIndex] = temp;
                        }
                        
                        Serial.print("Forecast entry for day ");
                        Serial.print(forecastIndex);
                        Serial.print(": temp=");
                        Serial.print(temp);
                        Serial.print("°F, condition=");
                        Serial.println(entry["weather"][0]["main"].as<const char*>());
                    }
                    
                    // Update forecast array with the highest and lowest temperatures
                    for (int i = 0; i < 5; i++) {
                        if (maxTempForDay[i] > -999) {
                            forecast[i].temp = round(maxTempForDay[i]);
                            forecast[i].iconType = getWeatherIconType(conditionForDay[i]);
                            
                            if (minTempForDay[i] < 999) {
                                forecast[i].lowTemp = round(minTempForDay[i]);
                            }
                            
                            Serial.print("Final forecast for ");
                            Serial.print(forecast[i].day);
                            Serial.print(": ");
                            Serial.print(forecast[i].temp);
                            Serial.print("°F (High), ");
                            Serial.print(forecast[i].lowTemp);
                            Serial.print("°F (Low), Icon: ");
                            Serial.println(forecast[i].iconType);
                        }
                    }
                    
                    lastWeatherUpdate = millis();
                    http.end();
                    return true;
                } else {
                    Serial.print("Forecast HTTP error: ");
                    Serial.println(httpCode);
                }
            } else {
                Serial.print("Failed to parse current weather JSON: ");
                Serial.println(error.c_str());
            }
        } else {
            Serial.print("Current weather HTTP error: ");
            Serial.println(httpCode);
            
            // Display error message if city not found (404)
            if (httpCode == 404) {
                u8g2.clearBuffer();
                u8g2.setFont(u8g2_font_6x10_tf);
                u8g2.drawStr(0, 10, "City not found!");
                u8g2.drawStr(0, 25, "Please update settings");
                u8g2.drawStr(0, 40, "at config portal");
                u8g2.sendBuffer();
                delay(3000);
            }
        }
        
        http.end();
        return false;
    }
}