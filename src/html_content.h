/*
 * HTML content for the web pages
 */

#ifndef HTML_CONTENT_H
#define HTML_CONTENT_H

#include <Arduino.h>

// Common HTML header with styles
const char HTML_HEADER[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>ESP Weather Display</title>
  <style>
    body { 
      font-family: Arial, sans-serif; 
      margin: 0; 
      padding: 20px; 
      background: #f5f5f5; 
      color: #333; 
    }
    h1 { color: #2c3e50; }
    .container { 
      max-width: 500px; 
      margin: 0 auto; 
      background: white; 
      padding: 20px; 
      border-radius: 5px; 
      box-shadow: 0 2px 5px rgba(0,0,0,0.1); 
    }
    label { 
      display: block; 
      margin-top: 10px; 
      font-weight: bold; 
    }
    select, input { 
      width: 100%; 
      padding: 8px; 
      margin-top: 5px; 
      margin-bottom: 15px; 
      border: 1px solid #ddd; 
      border-radius: 3px; 
    }
    button { 
      background: #3498db; 
      color: white; 
      border: none; 
      padding: 10px 15px; 
      border-radius: 3px; 
      cursor: pointer; 
    }
    button:hover { 
      background: #2980b9; 
    }
    #networks { 
      max-height: 150px; 
      overflow-y: auto; 
    }
    .nav-link { 
      display: inline-block; 
      margin-top: 20px; 
      color: #3498db; 
      text-decoration: none; 
    }
    .nav-link:hover { 
      text-decoration: underline; 
    }
  </style>
)rawliteral";

// WiFi Configuration page
const char WIFI_CONFIG_HTML[] PROGMEM = R"rawliteral(
  <script>
  function scanWifi() {
    document.getElementById('scanBtn').disabled = true;
    document.getElementById('scanBtn').innerText = 'Scanning...';
    fetch('/scan')
      .then(response => response.json())
      .then(data => {
        const select = document.getElementById('ssid');
        select.innerHTML = '';
        const currentSSID = '%CURRENT_SSID%';
        data.forEach(network => {
          const option = document.createElement('option');
          option.value = network.ssid;
          option.text = network.ssid + ' (' + network.rssi + ' dBm)';
          if (network.ssid === currentSSID) {
            option.selected = true;
          }
          select.appendChild(option);
        });
        document.getElementById('scanBtn').disabled = false;
        document.getElementById('scanBtn').innerText = 'Scan';
      });
  }
  
  window.onload = function() {
    scanWifi();
  };
  </script>
</head>
<body>
  <div class='container'>
    <h1>ESP Weather Configuration</h1>
    
    <form action='/save' method='POST'>
      <div id='wifi-config'>
        <h2>WiFi Settings</h2>
        
        <label for='ssid'>WiFi Network:</label>
        <div id='networks'>
          <select id='ssid' name='ssid'>
            <option value=''>Scanning...</option>
          </select>
        </div>
        <button type='button' id='scanBtn' onclick='scanWifi()'>Scan</button><br>
        
        <label for='password'>WiFi Password:</label>
        <input type='password' id='password' name='password' value=''><br>
      </div>
      
      <button type='submit'>Save WiFi Configuration</button>
    </form>
    
    <a href='/settings' class='nav-link'>Weather Display Settings →</a>
  </div>
</body>
</html>
)rawliteral";

// Weather Settings page
const char WEATHER_SETTINGS_HTML[] PROGMEM = R"rawliteral(
  <script>
  function populateStates() {
    const states = [
      'AL', 'AK', 'AZ', 'AR', 'CA', 'CO', 'CT', 'DE', 'FL', 'GA',
      'HI', 'ID', 'IL', 'IN', 'IA', 'KS', 'KY', 'LA', 'ME', 'MD',
      'MA', 'MI', 'MN', 'MS', 'MO', 'MT', 'NE', 'NV', 'NH', 'NJ',
      'NM', 'NY', 'NC', 'ND', 'OH', 'OK', 'OR', 'PA', 'RI', 'SC',
      'SD', 'TN', 'TX', 'UT', 'VT', 'VA', 'WA', 'WV', 'WI', 'WY'
    ];
    const stateSelect = document.getElementById('state');
    const currentState = '%STATE%'; // Will be replaced with actual state
    states.forEach(state => {
      const option = document.createElement('option');
      option.value = state;
      option.text = state;
      if (state === currentState) {
        option.selected = true;
      }
      stateSelect.appendChild(option);
    });
  }
  
  function populateTimezones() {
    const timezones = [
      {value: '-12', text: '(UTC-12:00) International Date Line West'},
      {value: '-11', text: '(UTC-11:00) Coordinated Universal Time-11'},
      {value: '-10', text: '(UTC-10:00) Hawaii'},
      {value: '-9', text: '(UTC-09:00) Alaska'},
      {value: '-8', text: '(UTC-08:00) Pacific Time (US & Canada)'},
      {value: '-7', text: '(UTC-07:00) Mountain Time (US & Canada)'},
      {value: '-6', text: '(UTC-06:00) Central Time (US & Canada)'},
      {value: '-5', text: '(UTC-05:00) Eastern Time (US & Canada)'},
      {value: '-4', text: '(UTC-04:00) Atlantic Time (Canada)'},
      {value: '-3.5', text: '(UTC-03:30) Newfoundland'},
      {value: '-3', text: '(UTC-03:00) Brasilia'},
      {value: '-2', text: '(UTC-02:00) Mid-Atlantic'},
      {value: '-1', text: '(UTC-01:00) Azores'},
      {value: '0', text: '(UTC+00:00) London, Dublin'},
      {value: '1', text: '(UTC+01:00) Paris, Berlin, Rome'},
      {value: '2', text: '(UTC+02:00) Athens, Istanbul'},
      {value: '3', text: '(UTC+03:00) Moscow'},
      {value: '3.5', text: '(UTC+03:30) Tehran'},
      {value: '4', text: '(UTC+04:00) Dubai'},
      {value: '4.5', text: '(UTC+04:30) Kabul'},
      {value: '5', text: '(UTC+05:00) Karachi'},
      {value: '5.5', text: '(UTC+05:30) New Delhi'},
      {value: '5.75', text: '(UTC+05:45) Kathmandu'},
      {value: '6', text: '(UTC+06:00) Dhaka'},
      {value: '6.5', text: '(UTC+06:30) Yangon'},
      {value: '7', text: '(UTC+07:00) Bangkok'},
      {value: '8', text: '(UTC+08:00) Singapore, Beijing'},
      {value: '9', text: '(UTC+09:00) Tokyo'},
      {value: '9.5', text: '(UTC+09:30) Adelaide'},
      {value: '10', text: '(UTC+10:00) Sydney'},
      {value: '11', text: '(UTC+11:00) Solomon Islands'},
      {value: '12', text: '(UTC+12:00) Auckland'},
      {value: '13', text: '(UTC+13:00) Samoa'},
      {value: '14', text: '(UTC+14:00) Line Islands'}
    ];
    
    const tzSelect = document.getElementById('timezone');
    const currentTz = '%TIMEZONE%';
    timezones.forEach(tz => {
      const option = document.createElement('option');
      option.value = tz.value;
      option.text = tz.text;
      if (tz.value === currentTz) {
        option.selected = true;
      }
      tzSelect.appendChild(option);
    });
  }
  
  window.onload = function() {
    populateStates();
    populateTimezones();
  };
  </script>
</head>
<body>
  <div class='container'>
    <h1>Weather Display Settings</h1>
    
    <form action='/settingssave' method='POST'>
      <div id='wifi-status'>
        <h2>Current WiFi Configuration</h2>
        <div style='background: #f8f9fa; padding: 15px; border-radius: 5px; margin-bottom: 20px;'>
          <p><strong>SSID:</strong> %WIFI_SSID%</p>
          <p style='font-size: 0.8em; color: #666;'>
            To change WiFi settings, use the WiFi Configuration page.
          </p>
        </div>
      </div>
      
      <div id='location-config'>
        <h2>Location Settings</h2>
        
        <label for='city'>City:</label>
        <input type='text' id='city' name='city' value='%CITY%'><br>
        
        <label for='state'>State:</label>
        <select id='state' name='state'>
        </select><br>
        
        <h2>Time Settings</h2>
        <label for='timezone'>Timezone:</label>
        <select id='timezone' name='timezone'>
        </select><br>
        
        <label for='timeFormat'>Time Format:</label>
        <select id='timeFormat' name='timeFormat'>
          <option value='0' %24HOUR_SELECTED%>24-hour (e.g., 14:30)</option>
          <option value='1' %12HOUR_SELECTED%>12-hour (e.g., 2:30 PM)</option>
        </select><br>
        
        <h2>Update Settings</h2>
        <label for='interval'>Weather Update Interval (minutes):</label>
        <select id='interval' name='interval'>
          %INTERVALS%
        </select><br>
      </div>
      
      <div id='api-config'>
        <h2>API Settings</h2>
        <label for='apikey'>OpenWeatherMap API Key:</label>
        <input type='text' id='apikey' name='apikey' value='%API_KEY%'><br>
        <p style='font-size: 0.8em; color: #666;'>
          To get your own API key, sign up at 
          <a href='https://openweathermap.org/api' target='_blank'>openweathermap.org</a>
          and create a free API key. The free tier allows up to 1,000 calls per day.
        </p>
      </div>
      
      <button type='submit'>Save Settings</button>
    </form>
    
    <a href='/' class='nav-link'>Back to WiFi Settings</a>
  </div>
</body>
</html>
)rawliteral";

// Success message for WiFi save
const char WIFI_SAVE_SUCCESS_HTML[] PROGMEM = R"rawliteral(
</head>
<body>
  <div class='container'>
    <h1>Configuration Saved!</h1>
    <p>Device will now connect to your WiFi network.</p>
    <p>If connection fails, the configuration portal will reopen.</p>
    <p>Debug Information:</p>
    <p>SSID: %SSID%</p>
    <p>Password: %PASSWORD%</p>
    <p>Password Length: %PASSLEN%</p>
  </div>
</body>
</html>
)rawliteral";

// Success message for settings save
const char SETTINGS_SAVE_SUCCESS_HTML[] PROGMEM = R"rawliteral(
  <meta http-equiv='refresh' content='5;url=/settings'>
</head>
<body>
  <div class='container'>
    <h1>Settings Saved!</h1>
    <p>Your weather display settings have been updated.</p>
    <p>Location: %CITY%, %STATE%</p>
    <p>Timezone: %TIMEZONE_TEXT%</p>
    <p>Time format: %TIME_FORMAT%</p>
    <p>Update interval: %INTERVAL% minutes</p>
    <p>API Key: %API_KEY_MASKED%</p>
    <p>Weather data will be updated with new settings.</p>
    <p>Redirecting back to settings page...</p>
  </div>
</body>
</html>
)rawliteral";

#endif // HTML_CONTENT_H
