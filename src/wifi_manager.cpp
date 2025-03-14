/*
 * Implementation of WiFi Manager functions (without encryption)
 */

#include "wifi_manager.h"
#include "html_content.h"
#include "time_manager.h"
#include <EEPROM.h>
#include <cstring>

// Load WiFi configuration from EEPROM
bool loadWiFiConfig() {
  // Add delay at startup to make serial monitoring easier
  delay(1000);
  Serial.println("\n\n============ LOADING WIFI CONFIG ============");
  
  EEPROM.begin(EEPROM_SIZE);
  
  // Check if configuration flag is set
  bool isConfigured = (EEPROM.read(CONFIG_FLAG_OFFSET) == 1);
  
  Serial.print("Config flag: ");
  Serial.println(isConfigured ? "SET (1)" : "NOT SET (0)");
  
  if (!isConfigured) {
    Serial.println("No configuration found in EEPROM");
    EEPROM.end();
    return false;
  }
  
  // Read the stored lengths
  uint8_t ssidLen = EEPROM.read(WIFI_SSID_OFFSET - 1);
  uint8_t passLen = EEPROM.read(WIFI_PASS_OFFSET - 1);
  Serial.printf("Stored SSID length: %d\n", ssidLen);
  Serial.printf("Stored password length: %d\n", passLen);
  
  // Read SSID and password directly (no encryption)
  char ssid[32] = {0};
  char password[64] = {0};
  
  Serial.println("Reading WiFi credentials from EEPROM...");
  
  // Read the SSID
  for (int i = 0; i < 32; i++) {
    byte b = EEPROM.read(WIFI_SSID_OFFSET + i);
    ssid[i] = b;
    if (i < ssidLen) {
      Serial.printf("%02X ", b);
    }
    if (i % 8 == 7 && i < ssidLen) Serial.println();
  }
  Serial.println();
  
  // Read the password
  Serial.println("Raw password bytes from EEPROM (hex, first 16 bytes):");
  for (int i = 0; i < 16; i++) {
    byte b = EEPROM.read(WIFI_PASS_OFFSET + i);
    password[i] = b;
    if (i < passLen) {
      Serial.printf("%02X ", b);
    }
    if (i % 8 == 7 && i < 16) Serial.println();
  }
  Serial.println();
  
  // Read the rest of the password
  for (int i = 16; i < 64; i++) {
    password[i] = EEPROM.read(WIFI_PASS_OFFSET + i);
  }
  
  EEPROM.end();
  
  // Ensure proper null termination using the stored lengths
  if (ssidLen > 0 && ssidLen < 32) {
    ssid[ssidLen] = '\0';
  }
  
  if (passLen > 0 && passLen < 64) {
    password[passLen] = '\0';
  }
  
  // Show the results on the display for debugging
  drawConnectingScreen("Loading credentials", "SSID: " + String(ssid));
  delay(2000);
  
  Serial.println("Credentials loaded:");
  Serial.print("SSID: '");
  Serial.print(ssid);
  Serial.println("'");
  Serial.print("Password length: ");
  Serial.println(strlen(password));
  
  // Make sure we have valid credentials
  if (strlen(ssid) == 0 || !isprint(ssid[0])) {
    Serial.println("ERROR: No valid SSID found, returning to setup mode");
    return false;
  }
  
  // Display SSID for debugging
  Serial.println("============ Preparing to connect ============");
  Serial.print("Will attempt to connect to SSID: '");
  Serial.print(ssid);
  Serial.println("'");
  Serial.print("With password of length: ");
  Serial.println(strlen(password));
  
  // Wait for a moment to let user read the debug info
  Serial.println("Starting connection process in 2 seconds...");
  delay(2000);
  
  // Set STA mode explicitly
  WiFi.mode(WIFI_STA);
  
  // Try to disconnect first if already connected
  WiFi.disconnect();
  delay(100);
  
  // Attempt to connect to saved WiFi
  Serial.println("Starting WiFi connection...");
  WiFi.begin(ssid, password);
  
  return true;
}

// Save WiFi configuration to EEPROM without encryption
void saveWiFiConfig(const char* ssid, const char* password) {
  Serial.println("\n\n============ SAVING WIFI CONFIG ============");
  
  // Store original credentials length for debugging
  size_t ssidLen = strlen(ssid);
  size_t passLen = strlen(password);
  
  // Debug output
  Serial.println("Saving WiFi configuration to EEPROM:");
  Serial.print("Original SSID: '");
  Serial.print(ssid);
  Serial.println("'");
  Serial.print("SSID length: ");
  Serial.println(ssidLen);
  Serial.print("Password length: ");
  Serial.println(passLen);
  
  // Store these lengths for later retrieval
  uint8_t ssidLenByte = (uint8_t)min(ssidLen, (size_t)255);
  uint8_t passLenByte = (uint8_t)min(passLen, (size_t)255);
  
  // Debug output of SSID bytes (hex values)
  Serial.print("SSID bytes (hex): ");
  for (size_t i = 0; i < ssidLen; i++) {
    Serial.printf("%02X ", (byte)ssid[i]);
  }
  Serial.println();
  
  // Show saving on display
  drawConnectingScreen("Saving credentials", "SSID: " + String(ssid));
  delay(1000);
  
  EEPROM.begin(EEPROM_SIZE);
  
  // Save credential lengths first
  EEPROM.write(WIFI_SSID_OFFSET - 1, ssidLenByte);
  EEPROM.write(WIFI_PASS_OFFSET - 1, passLenByte);
  
  // Save configuration flag first
  EEPROM.write(CONFIG_FLAG_OFFSET, 1);
  
  // Write SSID directly
  for (int i = 0; i < 32; i++) {
    if (i < ssidLen) {
      EEPROM.write(WIFI_SSID_OFFSET + i, ssid[i]);
    } else {
      EEPROM.write(WIFI_SSID_OFFSET + i, 0);
    }
  }
  
  // Write password directly
  for (int i = 0; i < 64; i++) {
    if (i < passLen) {
      EEPROM.write(WIFI_PASS_OFFSET + i, password[i]);
    } else {
      EEPROM.write(WIFI_PASS_OFFSET + i, 0);
    }
  }
  
  // Make sure all writes complete
  if (EEPROM.commit()) {
    Serial.println("EEPROM commit successful");
  } else {
    Serial.println("ERROR: EEPROM commit failed!");
  }
  EEPROM.end();
  
  Serial.println("Saved credentials");
  
  // Verify the data was written correctly by reading it back
  Serial.println("Verifying saved data...");
  EEPROM.begin(EEPROM_SIZE);
  
  // Check config flag
  bool configFlag = EEPROM.read(CONFIG_FLAG_OFFSET) == 1;
  Serial.print("Config flag after save: ");
  Serial.println(configFlag ? "SET (1)" : "NOT SET (0)");
  
  // Check saved lengths
  uint8_t savedSsidLen = EEPROM.read(WIFI_SSID_OFFSET - 1);
  uint8_t savedPassLen = EEPROM.read(WIFI_PASS_OFFSET - 1);
  Serial.printf("Saved SSID length: %d (should be %d)\n", savedSsidLen, ssidLen);
  Serial.printf("Saved password length: %d (should be %d)\n", savedPassLen, passLen);
  
  // Read back a few bytes to verify
  Serial.println("First 8 bytes of saved SSID (hex):");
  for (int i = 0; i < 8; i++) {
    Serial.printf("%02X ", EEPROM.read(WIFI_SSID_OFFSET + i));
  }
  Serial.println();
  
  EEPROM.end();
  delay(1000);
}

// Connect to WiFi using saved credentials
void connectToWifi() {
  // Load WiFi configuration
  bool configured = loadWiFiConfig();
  
  if (!configured) {
    Serial.println("No WiFi configured, starting portal");
    startConfigPortal();
    return;
  }
  
  // Display connecting message
  drawConnectingScreen("Connecting to WiFi", "");
  
  Serial.println("\n============ CONNECTING TO WIFI ============");
  
  // Stop any existing server before changing modes
  server.stop();
  
  // Set STA mode explicitly
  WiFi.mode(WIFI_STA);
  
  // Wait up to 30 seconds for connection (increased from 20)
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 60) {
    delay(500);
    Serial.print(".");
    timeout++;
    
    // Update the connection progress on display
    if (timeout % 2 == 0) {
      String dots = "";
      for (int i = 0; i < (timeout / 2) % 4; i++) {
        dots += ".";
      }
      drawConnectingScreen("Connecting to WiFi", dots);
      
      // Every 10 seconds, show status code for debugging
      if (timeout % 20 == 0) {
        int status = WiFi.status();
        Serial.println();
        Serial.print("Current WiFi status: ");
        Serial.print(status);
        Serial.print(" (");
        switch (status) {
          case WL_IDLE_STATUS: Serial.print("IDLE"); break;
          case WL_NO_SSID_AVAIL: Serial.print("NO_SSID_AVAIL"); break;
          case WL_SCAN_COMPLETED: Serial.print("SCAN_COMPLETED"); break;
          case WL_CONNECTED: Serial.print("CONNECTED"); break;
          case WL_CONNECT_FAILED: Serial.print("CONNECT_FAILED"); break;
          case WL_CONNECTION_LOST: Serial.print("CONNECTION_LOST"); break;
          case WL_DISCONNECTED: Serial.print("DISCONNECTED"); break;
          default: Serial.print("UNKNOWN"); break;
        }
        Serial.println(")");
        
        // Show status on display
        drawConnectingScreen("WiFi Status:", String(status) + " - " + (status == WL_DISCONNECTED ? "DISCONNECTED" : 
                                               status == WL_NO_SSID_AVAIL ? "NO SSID" : 
                                               status == WL_CONNECT_FAILED ? "FAILED" : "TRYING"));
        delay(1000); // Show for a second
      }
    }
  }
  
  // If connected successfully
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Enhanced debugging for web server
    Serial.println("Starting web server on connected WiFi");
    setupWebServer();
    server.begin();
    Serial.println("Web server started at http://" + WiFi.localIP().toString());
    
    // Show successful connection on display
    drawConnectingScreen("Connected!", "IP: " + WiFi.localIP().toString());
    delay(2000);
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi");
    Serial.println("WiFi status code: " + String(WiFi.status()));
    Serial.println("Status meanings: 0=IDLE, 1=NO_SSID_AVAIL, 2=SCAN_COMPLETED, 3=CONNECTED");
    Serial.println("                 4=CONNECT_FAILED, 5=CONNECTION_LOST, 6=DISCONNECTED");
    
    // Show connection failure on display
    drawConnectingScreen("Connection failed!", "Status: " + String(WiFi.status()));
    delay(3000);
    
    Serial.println("Starting configuration portal due to connection failure");
    
    // Start configuration portal if connection fails
    startConfigPortal();
  }
}

// Start the configuration portal (AP mode)
void startConfigPortal() {
  // Stop any existing server before changing modes
  server.stop();
  
  // Set AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_NAME, AP_PASSWORD);
  
  // Configure DNS server to redirect all domains to AP IP
  dnsServer.start(DNS_PORT, "*", apIP);
  
  // Start web server
  setupWebServer();
  server.begin();
  
  Serial.println("Configuration Portal Started");
  Serial.print("SSID: ");
  Serial.println(AP_NAME);
  Serial.print("Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("IP address: ");
  Serial.println(apIP);
  
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
    Serial.print("SSID: '");
    Serial.print(ssid);
    Serial.println("'");
    Serial.print("Password length: ");
    Serial.println(password.length());
    
    // Save WiFi configuration
    saveWiFiConfig(ssid.c_str(), password.c_str());
    
    String html = FPSTR(HTML_HEADER);
    html.replace("</head>", "<meta http-equiv='refresh' content='10;url=/'></head>");
    
    String successHtml = FPSTR(WIFI_SAVE_SUCCESS_HTML);
    successHtml.replace("%SSID%", ssid);
    successHtml.replace("%PASSLEN%", String(password.length()));
    
    html += successHtml;
    server.send(200, "text/html", html);
    
    delay(1000);
    
    // Clear any existing connections
    WiFi.disconnect(true);
    delay(1000);
    
    // Stop the server before changing modes
    server.stop();
    
    // Try a simple direct connection first
    Serial.println("Attempting direct connection with new credentials");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    drawConnectingScreen("Connecting to WiFi", "");
    
    int directTimeout = 0;
    while (WiFi.status() != WL_CONNECTED && directTimeout < 20) {
      delay(500);
      Serial.print(".");
      directTimeout++;
      
      if (directTimeout % 2 == 0) {
        String dots = "";
        for (int i = 0; i < (directTimeout / 2) % 4; i++) {
          dots += ".";
        }
        drawConnectingScreen("Connecting to WiFi", dots);
      }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nDirect connection successful!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      
      // Restart the web server in STA mode
      setupWebServer();
      server.begin();
      Serial.println("Web server restarted at http://" + WiFi.localIP().toString());
      
      // Show connected info on display
      drawConnectingScreen("Connected!", "IP: " + WiFi.localIP().toString());
      delay(3000);
    } else {
      Serial.println("\nDirect connection failed, trying normal connection process");
      Serial.println("WiFi status code: " + String(WiFi.status()));
      // Fall back to normal connect process with saved credentials
      connectToWifi();
    }
  } else {
    server.send(400, "text/plain", "Missing SSID or password");
  }
}

// Handle settings page request
void handleSettings() {
  String html = FPSTR(HTML_HEADER);
  String settingsHtml = FPSTR(WEATHER_SETTINGS_HTML);
  
  // Replace placeholders
  settingsHtml.replace("%CITY%", cityName);
  settingsHtml.replace("%STATE%", stateName);
  settingsHtml.replace("%TIMEZONE%", String(timezone));
  
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
    
    // Save settings
    saveSettings(city, state, interval, tz);
    
    // Get timezone text for display
    String timezoneText = getTimezoneText(tz);
    
    String html = FPSTR(HTML_HEADER);
    String successHtml = FPSTR(SETTINGS_SAVE_SUCCESS_HTML);
    
    // Replace placeholders
    successHtml.replace("%CITY%", city);
    successHtml.replace("%STATE%", state);
    successHtml.replace("%INTERVAL%", String(interval / 60000));
    successHtml.replace("%TIMEZONE_TEXT%", timezoneText);
    
    html += successHtml;
    server.send(200, "text/html", html);
    
    // Force an immediate weather update with new settings
    lastWeatherUpdate = 0;
    
    // Force time update with new timezone
    lastTimeUpdate = 0;
    
    // Immediately update the time with the new timezone
    if (WiFi.status() == WL_CONNECTED) {
      resetTimeWithNewTimezone();
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
  
  Serial.println("Settings loaded from EEPROM:");
  Serial.println("City: " + cityName);
  Serial.println("State: " + stateName);
  Serial.println("Update interval: " + String(WEATHER_UPDATE_INTERVAL / 60000) + " minutes");
  Serial.println("Timezone: " + String(timezone) + " (UTC" + (timezone >= 0 ? "+" : "") + String(timezone) + ")");
}

// Save settings to EEPROM
void saveSettings(String city, String state, unsigned long updateInterval, float tz) {
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
  
  EEPROM.commit();
  EEPROM.end();
  
  // Update global variables
  cityName = city;
  stateName = state;
  WEATHER_UPDATE_INTERVAL = updateInterval;
  timezone = tz;
  
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