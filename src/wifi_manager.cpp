/*
 * Implementation of WiFi Manager functions
 */

#include "wifi_manager.h"
#include <EEPROM.h>
#include <U8g2lib.h>
#include "html_content.h"
#include "time_manager.h"

// Read WiFi credentials directly from EEPROM
WiFiCredentials readWiFiCredentialsFromEEPROM() {
  WiFiCredentials creds;
  
  EEPROM.begin(EEPROM_SIZE);
  
  // Read the stored lengths from the new locations
  uint8_t ssidLen = EEPROM.read(CONFIG_FLAG_OFFSET + 1);
  uint8_t passLen = EEPROM.read(CONFIG_FLAG_OFFSET + 2);
  
  Serial.println("Reading WiFi configuration...");
  
  // Read SSID
  char ssid[33] = {0};  // One extra byte for null terminator
  for (int i = 0; i < min(ssidLen, (uint8_t)32); i++) {
    ssid[i] = EEPROM.read(WIFI_SSID_OFFSET + i);
  }
  ssid[min(ssidLen, (uint8_t)32)] = '\0';
  
  // Read password
  char password[65] = {0};  // One extra byte for null terminator
  for (int i = 0; i < min(passLen, (uint8_t)64); i++) {
    password[i] = EEPROM.read(WIFI_PASS_OFFSET + i);
  }
  password[min(passLen, (uint8_t)64)] = '\0';
  
  creds.ssid = String(ssid);
  creds.password = String(password);
  
  Serial.println("WiFi configuration read successfully");
  
  return creds;
}

// Load WiFi configuration from EEPROM
bool loadWiFiConfig() {
  Serial.println("\n\n============ LOADING WIFI CONFIG ============");
  
  EEPROM.begin(EEPROM_SIZE);
  
  // Check if configuration flag is set
  bool isConfigured = (EEPROM.read(CONFIG_FLAG_OFFSET) == 1);
  
  if (!isConfigured) {
    Serial.println("No configuration found in EEPROM");
    EEPROM.end();
    return false;
  }
  
  // Read the stored lengths from the new locations
  uint8_t ssidLen = EEPROM.read(CONFIG_FLAG_OFFSET + 1);
  uint8_t passLen = EEPROM.read(CONFIG_FLAG_OFFSET + 2);
  
  // Validate lengths
  if (ssidLen == 0 || ssidLen > 32 || passLen > 64) {
    Serial.println("ERROR: Invalid configuration lengths");
    return false;
  }
  
  // Read SSID and password from EEPROM
  char ssid[33] = {0};  // One extra byte for null terminator
  char password[65] = {0};  // One extra byte for null terminator
  
  // Read the SSID
  for (int i = 0; i < ssidLen; i++) {
    ssid[i] = EEPROM.read(WIFI_SSID_OFFSET + i);
  }
  ssid[ssidLen] = '\0'; // Ensure null termination
  
  // Read the password
  for (int i = 0; i < passLen; i++) {
    password[i] = EEPROM.read(WIFI_PASS_OFFSET + i);
  }
  password[passLen] = '\0'; // Ensure null termination
  
  EEPROM.end();
  
  // Basic validation
  if (strlen(ssid) == 0) {
    Serial.println("ERROR: Empty SSID found, returning to setup mode");
    return false;
  }
  
  // Check if SSID contains non-printable characters
  bool hasNonPrintable = false;
  for (size_t i = 0; i < strlen(ssid); i++) {
    if (!isprint(ssid[i])) {
      hasNonPrintable = true;
      break;
    }
  }
  
  if (hasNonPrintable) {
    Serial.println("ERROR: SSID contains non-printable characters, returning to setup mode");
    return false;
  }
  
  Serial.println("Attempting to connect to WiFi network...");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  
  return true;
}

// Save WiFi configuration to EEPROM (simple, reliable approach)
void saveWiFiConfig(const char* ssid, const char* password) {
  EEPROM.begin(EEPROM_SIZE);
  
  // Basic validation
  if (ssid == nullptr || password == nullptr) {
    Serial.println("ERROR: Invalid parameters");
    EEPROM.end();
    return;
  }
  
  size_t ssidLen = strlen(ssid);
  size_t passLen = strlen(password);
  
  // Validate lengths
  if (ssidLen == 0 || ssidLen > 32) {
    Serial.println("ERROR: Invalid SSID length");
    EEPROM.end();
    return;
  }
  
  if (passLen > 64) {
    Serial.println("ERROR: Password too long");
    EEPROM.end();
    return;
  }
  
  Serial.println("Saving WiFi configuration...");
  
  // Write the configuration flag and lengths
  uint8_t ssidLenByte = (uint8_t)ssidLen;
  uint8_t passLenByte = (uint8_t)passLen;
  
  EEPROM.write(CONFIG_FLAG_OFFSET, 1);  // Use 1 as the config flag value
  EEPROM.write(CONFIG_FLAG_OFFSET + 1, ssidLenByte);
  EEPROM.write(CONFIG_FLAG_OFFSET + 2, passLenByte);
  
  // Write SSID and password
  for (size_t i = 0; i < ssidLen; i++) {
    EEPROM.write(WIFI_SSID_OFFSET + i, ssid[i]);
  }
  
  for (size_t i = 0; i < passLen; i++) {
    EEPROM.write(WIFI_PASS_OFFSET + i, password[i]);
  }
  
  // Clear any remaining bytes
  for (size_t i = ssidLen; i < 32; i++) {
    EEPROM.write(WIFI_SSID_OFFSET + i, 0);
  }
  for (size_t i = passLen; i < 64; i++) {
    EEPROM.write(WIFI_PASS_OFFSET + i, 0);
  }
  
  if (EEPROM.commit()) {
    Serial.println("WiFi configuration saved successfully");
  } else {
    Serial.println("ERROR: Failed to save configuration");
  }
}

// Connect to WiFi using saved credentials
void connectToWifi() {
  // Clean WiFi state and prepare for connection
  Serial.println("Preparing WiFi hardware...");
  WiFi.persistent(false);  // Prevent credentials from being written to flash
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);  // Increased delay to ensure WiFi hardware resets
  
  WiFi.mode(WIFI_STA);
  delay(200);  // Give hardware time to switch modes
  
  // Apply optimal settings for faster connection
  wifi_set_sleep_type(NONE_SLEEP_T);
  WiFi.setAutoReconnect(true);
  
  // Display connecting message
  drawConnectingScreen("Connecting to WiFi", "");
  
  // Load WiFi configuration
  Serial.println("Loading credentials from EEPROM...");
  bool configured = loadWiFiConfig();
  
  if (!configured) {
    Serial.println("No WiFi configured, starting portal");
    startConfigPortal();
    return;
  }
  
  Serial.println("Attempting to connect...");
  
  // Connection attempt with multiple retries
  const int MAX_RETRIES = 3;
  const unsigned long ATTEMPT_TIMEOUT = 20000; // 20 seconds per attempt
  bool connected = false;
  
  for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
    if (attempt > 0) {
      Serial.printf("\nRetry attempt %d of %d\n", attempt + 1, MAX_RETRIES);
      drawConnectingScreen("Retrying connection", String(attempt + 1) + " of " + String(MAX_RETRIES));
      delay(1000);
      
      // Reset WiFi connection for retry - with different approach for subsequent attempts
      Serial.println("Resetting WiFi state for retry");
      WiFi.disconnect(true);
      delay(200);
      
      // For odd-numbered attempts, use saved credentials
      // For even-numbered attempts, read from EEPROM again
      // This gives us two different methods of retry
      if (attempt % 2 == 0) {
        WiFi.begin(); // Use stored credentials from previous attempt
      } else {
        loadWiFiConfig(); // Reload from EEPROM
      }
    }
    
    unsigned long startAttempt = millis();
    int dotCount = 0;
    
    while (millis() - startAttempt < ATTEMPT_TIMEOUT) {
      // Check connection status
      wl_status_t status = WiFi.status();
      
      if (status == WL_CONNECTED) {
        connected = true;
        break;
      }
      
      // Display connection status for debugging
      if ((millis() - startAttempt) % 2000 < 100) {
        // Status code to string conversion directly inline
        const char* statusStr = "UNKNOWN";
        switch (status) {
          case WL_CONNECTED: statusStr = "CONNECTED"; break;
          case WL_NO_SHIELD: statusStr = "NO SHIELD"; break;
          case WL_IDLE_STATUS: statusStr = "IDLE"; break;
          case WL_NO_SSID_AVAIL: statusStr = "NO SSID AVAIL"; break;
          case WL_SCAN_COMPLETED: statusStr = "SCAN COMPLETED"; break;
          case WL_CONNECT_FAILED: statusStr = "CONNECT FAILED"; break;
          case WL_CONNECTION_LOST: statusStr = "CONNECTION LOST"; break;
          case WL_DISCONNECTED: statusStr = "DISCONNECTED"; break;
          case WL_WRONG_PASSWORD: statusStr = "WRONG PASSWORD"; break;
        }
        Serial.printf("Status: %d (%s)\n", status, statusStr);
      }
      
      // Update dots every 250ms for visual feedback
      if ((millis() - startAttempt) % 250 < 50) {
        dotCount = (dotCount + 1) % 4;
        String dots = "";
        for (int i = 0; i < dotCount; i++) dots += ".";
        drawConnectingScreen("Connecting to WiFi", dots);
      }
      
      delay(100); // Slightly longer delay to prevent thrashing
    }
    
    if (connected) break;
  }
  
  // If connected successfully
  if (connected) {
    Serial.println("\nWiFi connected successfully!");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    
    // Show successful connection on display
    drawConnectingScreen("Connected!", "IP: " + WiFi.localIP().toString());
    delay(1000);
    
    // Start web server
    server.stop(); // Stop any existing server
    setupWebServer();
    server.begin();
  } else {
    Serial.println("\nAll connection attempts failed, starting portal");
    drawConnectingScreen("Connection failed", "Starting portal...");
    delay(1000);
    startConfigPortal();
  }
}

// Format/clear WiFi credentials in EEPROM 
void formatCredentials() {
  Serial.println("Formatting WiFi credentials in EEPROM...");
  drawConnectingScreen("Formatting", "WiFi credentials");
  
  EEPROM.begin(EEPROM_SIZE);
  
  // Clear config flag first
  EEPROM.write(CONFIG_FLAG_OFFSET, 0);
  
  // Clear credential lengths in new locations
  EEPROM.write(CONFIG_FLAG_OFFSET + 1, 0);
  EEPROM.write(CONFIG_FLAG_OFFSET + 2, 0);
  
  // Clear SSID area
  for (int i = WIFI_SSID_OFFSET; i < WIFI_PASS_OFFSET; i++) {
    EEPROM.write(i, 0);
  }
  
  // Clear password area
  for (int i = WIFI_PASS_OFFSET; i < WIFI_PASS_OFFSET + 64; i++) {
    EEPROM.write(i, 0);
  }
  
  // Commit changes
  EEPROM.commit();
  EEPROM.end();
  
  Serial.println("Credentials formatted successfully");
  delay(1000);
}

// Start the configuration portal (AP mode)
void startConfigPortal() {
  // No automatic formatting - credentials should persist across portal sessions
  // formatCredentials();  // Removed automatic formatting
  
  Serial.println("Starting configuration portal WITHOUT formatting credentials");
  
  // Ensure we're disconnected from any STA mode connection
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  
  // Set AP+STA mode instead of AP-only mode to better control when connections happen
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_NAME, AP_PASSWORD);
  
  // Explicitly disconnect STA mode to ensure we don't automatically connect
  WiFi.disconnect(true);
  
  // Configure DNS server to redirect all domains to AP IP
  dnsServer.start(DNS_PORT, "*", apIP);
  
  // Stop any existing server and start web server
  server.stop();
  setupWebServer();
  server.begin();
  
  Serial.println("Configuration Portal Started");
  Serial.printf("SSID: %s, Password: %s\n", AP_NAME, AP_PASSWORD);
  Serial.printf("IP address: %s\n", apIP.toString().c_str());
  
  // Show configuration mode on display
  drawConfigMode();
}

// Setup the web server routes
void setupWebServer() {
  // Add debug handler to test server connectivity
  server.on("/debug", HTTP_GET, []() {
    Serial.println("DEBUG endpoint accessed");
    String debugInfo = "ESP8266 Web Server Debug Info\n\n";
    debugInfo += "WiFi Mode: " + String(WiFi.getMode() == WIFI_AP ? "Access Point" : "Station") + "\n";
    debugInfo += "IP Address: " + WiFi.localIP().toString() + "\n";
    debugInfo += "MAC Address: " + WiFi.macAddress() + "\n";
    debugInfo += "Free Heap: " + String(ESP.getFreeHeap()) + " bytes\n";
    debugInfo += "Uptime: " + String(millis() / 1000) + " seconds\n";
    debugInfo += "Active Connections: " + String(server.client().available()) + "\n";
    
    server.send(200, "text/plain", debugInfo);
  });
  
  // Add a simple test endpoint that just returns "OK"
  server.on("/test", HTTP_GET, []() {
    Serial.println("TEST endpoint accessed");
    server.send(200, "text/plain", "OK - Web server is running!");
  });
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/scan", HTTP_GET, handleWifiScan);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/settingssave", HTTP_POST, handleSettingsSave);
  
  // Handle not found (404) - redirect to root
  server.onNotFound([]() {
    Serial.println("404 Not Found: " + server.uri());
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });
}

// Handle root page request
void handleRoot() {
  String html = FPSTR(HTML_HEADER);
  html += FPSTR(WIFI_CONFIG_HTML);
  
  // Replace placeholder with current SSID if connected
  if (WiFi.status() == WL_CONNECTED) {
    html.replace("%CURRENT_SSID%", WiFi.SSID());
  } else {
    html.replace("%CURRENT_SSID%", "");
  }
  
  server.send(200, "text/html", html);
}

// Handle WiFi network scanning
void handleWifiScan() {
  // Scan for WiFi networks
  int numNetworks = WiFi.scanNetworks();
  String jsonResponse = "[";
  
  for (int i = 0; i < numNetworks; i++) {
    if (i > 0) jsonResponse += ",";
    jsonResponse += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  
  jsonResponse += "]";
  server.send(200, "application/json", jsonResponse);
  
  // Free memory by deleting scan results
  WiFi.scanDelete();
}

// Handle save configuration request
void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    // Debug output
    Serial.println("Saving WiFi configuration:");
    Serial.printf("SSID: '%s', Password length: %d\n", ssid.c_str(), password.length());
    
    // Show saving on display
    drawConnectingScreen("Saving credentials", "Please wait...");
    
    // Save WiFi configuration
    saveWiFiConfig(ssid.c_str(), password.c_str());
    
    // Send success page to client
    String html = FPSTR(HTML_HEADER);
    html.replace("</head>", "<meta http-equiv='refresh' content='5;url=/'></head>");
    
    String successHtml = FPSTR(WIFI_SAVE_SUCCESS_HTML);
    successHtml.replace("%SSID%", ssid);
    successHtml.replace("%PASSWORD%", password);
    successHtml.replace("%PASSLEN%", String(password.length()));
    
    html += successHtml;
    server.send(200, "text/html", html);
    
    // Wait a moment to ensure page is sent
    delay(1000);
    
    // Prepare for connection attempt with the new credentials
    Serial.println("Shutting down AP and attempting to connect with new credentials");
    
    // Properly shut down AP mode
    WiFi.softAPdisconnect(true);
    dnsServer.stop();
    
    // Clear all WiFi settings to ensure a clean state
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500);
    
    // Start fresh with new credentials in STA mode
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Display connecting screen
    drawConnectingScreen("Connecting", "with new credentials");
    
    // Connection attempt - 15 second timeout
    unsigned long startAttempt = millis();
    bool connected = false;
    
    // More robust connection attempt with visual feedback
    while (millis() - startAttempt < 15000) {
      if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        break;
      }
      
      // Visual feedback
      int dots = ((millis() - startAttempt) / 500) % 4;
      String dotStr = "";
      for (int i = 0; i < dots; i++) dotStr += ".";
      drawConnectingScreen("Connecting", dotStr);
      
      delay(100);
    }
    
    if (connected) {
      Serial.println("Connected successfully with new credentials!");
      Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
      
      // Show connection success
      drawConnectingScreen("Connected!", "IP: " + WiFi.localIP().toString());
      delay(1000);
      
      // Restart web server in STA mode
      server.stop();
      setupWebServer();
      server.begin();
    } else {
      Serial.println("Failed to connect with new credentials, restarting device");
      drawConnectingScreen("Connection failed", "Restarting device...");
      delay(2000);
      
      // Force a complete restart as a last resort to get to a clean state
      ESP.restart();
    }
  } else {
    server.send(400, "text/plain", "Missing SSID or password");
  }
}

// Handle settings page request
void handleSettings() {
  String html = FPSTR(HTML_HEADER);
  String settingsHtml = FPSTR(WEATHER_SETTINGS_HTML);
  
  // Get WiFi credentials from EEPROM
  WiFiCredentials creds = readWiFiCredentialsFromEEPROM();
  
  Serial.println("\n=== Handling Settings Page Request ===");
  Serial.printf("SSID to display: '%s'\n", creds.ssid.c_str());
  Serial.printf("Password length to display: %d\n", creds.password.length());
  
  // Replace placeholders
  settingsHtml.replace("%CITY%", cityName);
  settingsHtml.replace("%STATE%", stateName);
  settingsHtml.replace("%WIFI_SSID%", creds.ssid);
  
  // Create a string of asterisks matching the password length
  String maskedPassword;
  for (size_t i = 0; i < creds.password.length(); i++) {
    maskedPassword += "*";
  }
  settingsHtml.replace("%WIFI_PASSWORD%", maskedPassword);
  
  // Format timezone as a string with correct precision for matching with JavaScript values
  String tzStr;
  if (timezone == (int)timezone) {
    tzStr = String((int)timezone); // Use integer representation for whole numbers
  } else {
    tzStr = String(timezone, 1); // Use 1 decimal place for fractional timezones
  }
  settingsHtml.replace("%TIMEZONE%", tzStr);
  
  settingsHtml.replace("%API_KEY%", API_KEY);
  
  // Generate interval options
  String intervalOptions = "";
  int intervals[] = {1, 5, 10, 15, 30, 60};
  int currentInterval = WEATHER_UPDATE_INTERVAL / 60000;
  
  for (int i = 0; i < 6; i++) {
    intervalOptions += "<option value='" + String(intervals[i]) + "'";
    if (intervals[i] == currentInterval) {
      intervalOptions += " selected";
    }
    intervalOptions += ">" + String(intervals[i]) + " minute";
    if (intervals[i] > 1) intervalOptions += "s";
    intervalOptions += "</option>";
  }
  
  settingsHtml.replace("%INTERVALS%", intervalOptions);
  
  Serial.println("Placeholders replaced in template");
  Serial.println("================================\n");
  
  html += settingsHtml;
  server.send(200, "text/html", html);
}

// Handle settings save request
void handleSettingsSave() {
  if (server.hasArg("city") && server.hasArg("state") && server.hasArg("interval") && server.hasArg("timezone")) {
    String city = server.arg("city");
    String state = server.arg("state");
    unsigned long interval = server.arg("interval").toInt() * 60000; // Convert minutes to milliseconds
    float tz = server.arg("timezone").toFloat();
    String apiKey = server.hasArg("apikey") ? server.arg("apikey") : API_KEY;
    
    // Store the old API key to check if it changed
    String oldApiKey = API_KEY;
    bool apiKeyChanged = (apiKey.length() >= 5 && apiKey != oldApiKey);
    
    // Validate city name
    if (city.length() == 0) {
      city = "New York"; // Default if empty
      Serial.println("Empty city name provided, using default 'New York'");
    }
    
    // Validate state code
    if (state.length() != 2) {
      state = "NY"; // Default if invalid
      Serial.println("Invalid state code provided, using default 'NY'");
    }
    
    // Validate timezone
    if (tz < -12 || tz > 14) {
      tz = -5.0; // Default to Eastern Time if invalid
      Serial.println("Invalid timezone provided, using default '-5'");
    }
    
    // Validate API key - must be at least 5 characters
    if (apiKey.length() < 5) {
      apiKey = API_KEY; // Keep using current key if invalid
      Serial.println("Invalid API key provided, keeping current API key");
    }
    
    // Save settings
    saveSettings(city, state, interval, tz, apiKey);
    
    // Get timezone text for display
    String timezoneText = getTimezoneText(tz);
    
    String html = FPSTR(HTML_HEADER);
    String successHtml = FPSTR(SETTINGS_SAVE_SUCCESS_HTML);
    
    // Replace placeholders
    successHtml.replace("%CITY%", city);
    successHtml.replace("%STATE%", state);
    successHtml.replace("%INTERVAL%", String(interval / 60000));
    successHtml.replace("%TIMEZONE_TEXT%", timezoneText);
    
    // Mask API key for security - show first 4 and last 4 characters
    String maskedApiKey;
    if (apiKey.length() > 8) {
      maskedApiKey = apiKey.substring(0, 4) + "********" + apiKey.substring(apiKey.length() - 4);
    } else {
      maskedApiKey = "********"; // If API key is too short to mask properly
    }
    successHtml.replace("%API_KEY_MASKED%", maskedApiKey);
    
    html += successHtml;
    server.send(200, "text/html", html);
    
    // Force an immediate weather update with new settings
    lastWeatherUpdate = 0;
    
    // Force time update with new timezone
    lastTimeUpdate = 0;
    
    // Immediately update the time with the new timezone
    if (WiFi.status() == WL_CONNECTED) {
      resetTimeWithNewTimezone();
      
      // If API key was changed and is valid, fetch weather data immediately
      if (apiKeyChanged) {
        Serial.println("New API key detected - fetching weather data immediately");
        drawConnectingScreen("New API Key", "Fetching weather...");
        Weather::fetchWeatherData();
      }
    }
  } else {
    server.send(400, "text/plain", "Missing required parameters");
  }
}

// Helper function to get timezone text
String getTimezoneText(float tz) {
  String prefix = tz >= 0 ? "+" : "";
  String hours = String(int(tz));
  String minutes = "";
  
  // Handle fractional timezones (like India's UTC+5:30)
  float fraction = abs(tz) - int(abs(tz));
  if (fraction > 0) {
    minutes = ":" + String(int(fraction * 60));
  }
  
  return "UTC" + prefix + hours + minutes;
}

// Load settings from EEPROM
void loadSettings() {
  EEPROM.begin(EEPROM_SIZE);
  
  // Load city name
  char city[50] = {0};
  for (int i = 0; i < 50; i++) {
    city[i] = EEPROM.read(CITY_OFFSET + i);
    if (city[i] == 0) break;
  }
  
  // Load state code
  char state[3] = {0};
  for (int i = 0; i < 2; i++) {
    state[i] = EEPROM.read(STATE_OFFSET + i);
  }
  
  // Load update interval
  byte intervalBytes[4];
  for (int i = 0; i < 4; i++) {
    intervalBytes[i] = EEPROM.read(UPDATE_INTERVAL_OFFSET + i);
  }
  
  // Load timezone
  byte timezoneBytes[4];
  for (int i = 0; i < 4; i++) {
    timezoneBytes[i] = EEPROM.read(TIMEZONE_OFFSET + i);
  }
  
  // Load API key
  char apiKey[50] = {0};
  for (int i = 0; i < 50; i++) {
    apiKey[i] = EEPROM.read(API_KEY_OFFSET + i);
    if (apiKey[i] == 0) break;
  }
  
  EEPROM.end();
  
  // Convert bytes to values
  unsigned long interval = 0;
  memcpy(&interval, intervalBytes, 4);
  
  float tz = 0;
  memcpy(&tz, timezoneBytes, 4);
  
  // Validate and set values
  if (strlen(city) > 0 && city[0] != 255) {
    cityName = String(city);
  }
  
  if (strlen(state) == 2 && state[0] != 255) {
    stateName = String(state);
  }
  
  if (interval > 0 && interval < 24 * 60 * 60 * 1000) { // Less than 24 hours
    WEATHER_UPDATE_INTERVAL = interval;
  }
  
  if (tz >= -12 && tz <= 14) {
    timezone = tz;
  }
  
  if (strlen(apiKey) >= 5 && apiKey[0] != 255) {
    API_KEY = String(apiKey);
  }
  
  Serial.println("Settings loaded from EEPROM:");
  Serial.println("City: " + cityName);
  Serial.println("State: " + stateName);
  Serial.println("Update interval: " + String(WEATHER_UPDATE_INTERVAL / 60000) + " minutes");
  Serial.println("Timezone: " + String(timezone) + " (UTC" + (timezone >= 0 ? "+" : "") + String(timezone) + ")");
}

// Save settings to EEPROM
void saveSettings(String city, String state, unsigned long updateInterval, float tz, String apiKey) {
  EEPROM.begin(EEPROM_SIZE);
  
  // Save city name
  for (size_t i = 0; i < 50; i++) {
    if (i < city.length()) {
      EEPROM.write(CITY_OFFSET + i, city[i]);
    } else {
      EEPROM.write(CITY_OFFSET + i, 0);
    }
  }
  
  // Save state code
  for (size_t i = 0; i < 2; i++) {
    if (i < state.length()) {
      EEPROM.write(STATE_OFFSET + i, state[i]);
    } else {
      EEPROM.write(STATE_OFFSET + i, 0);
    }
  }
  
  // Save update interval
  byte intervalBytes[4];
  memcpy(intervalBytes, &updateInterval, 4);
  for (int i = 0; i < 4; i++) {
    EEPROM.write(UPDATE_INTERVAL_OFFSET + i, intervalBytes[i]);
  }
  
  // Save timezone
  byte timezoneBytes[4];
  memcpy(timezoneBytes, &tz, 4);
  for (int i = 0; i < 4; i++) {
    EEPROM.write(TIMEZONE_OFFSET + i, timezoneBytes[i]);
  }
  
  // Save API key
  for (size_t i = 0; i < 50; i++) {
    if (i < apiKey.length()) {
      EEPROM.write(API_KEY_OFFSET + i, apiKey[i]);
    } else {
      EEPROM.write(API_KEY_OFFSET + i, 0);
    }
  }
  
  EEPROM.commit();
  EEPROM.end();
  
  // Update global variables
  cityName = city;
  stateName = state;
  WEATHER_UPDATE_INTERVAL = updateInterval;
  timezone = tz;
  API_KEY = apiKey;
  
  Serial.println("Settings saved to EEPROM:");
  Serial.println("City: " + cityName);
  Serial.println("State: " + stateName);
  Serial.println("Update interval: " + String(WEATHER_UPDATE_INTERVAL / 60000) + " minutes");
  Serial.println("Timezone: " + String(timezone) + " (UTC" + (timezone >= 0 ? "+" : "") + String(timezone) + ")");
}

// Draw the connecting screen with progress
void drawConnectingScreen(String message, String submessage) {
  u8g2.clearBuffer();
  
  // Draw main message
  u8g2.setFont(u8g2_font_t0_11_tf);
  int msgWidth = u8g2.getStrWidth(message.c_str());
  u8g2.drawStr(64 - msgWidth / 2, 25, message.c_str());
  
  // Draw submessage on next line
  if (submessage.length() > 0) {
    int subWidth = u8g2.getStrWidth(submessage.c_str());
    u8g2.drawStr(64 - subWidth / 2, 40, submessage.c_str());
  }
  
  u8g2.sendBuffer();
}

// Draw the configuration mode screen
void drawConfigMode() {
  u8g2.clearBuffer();
  
  // Draw title
  u8g2.setFont(u8g2_font_t0_11_tf);
  String title = "WiFi Setup Mode";
  int titleWidth = u8g2.getStrWidth(title.c_str());
  u8g2.drawStr(64 - titleWidth / 2, 12, title.c_str());
  
  // Draw instructions with smaller font to fit more text
  u8g2.setFont(u8g2_font_tom_thumb_4x6_mf);
  
  u8g2.drawStr(5, 22, "Connect to WiFi:");
  u8g2.drawStr(5, 30, AP_NAME);
  
  u8g2.drawStr(5, 38, "Password:");
  u8g2.drawStr(5, 46, AP_PASSWORD);
  
  u8g2.drawStr(5, 54, "Then visit:");
  u8g2.drawStr(5, 62, "192.168.4.1");
  
  u8g2.sendBuffer();
}