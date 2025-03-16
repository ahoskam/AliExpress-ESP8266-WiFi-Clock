# WiFi Clock - Open Source Alternative

This is an open-source project that provides alternative firmware for the ESP8266-based WiFi Clock display commonly found on AliExpress ([like this one](https://www.aliexpress.us/item/3256806715737170.html)). This project aims to provide users with a transparent, customizable, and secure alternative to the original closed-source Chinese software.

## Features

- Easy WiFi configuration through a web interface
- Real-time clock synchronization using NTP
- Three display screens:
  1. Time Screen
     - Current time display
     - Visual progression from sunrise to sunset
     - Sunset to sunrise progression (currently under development)
  2. Current Weather Screen
     - Current temperature
     - Weather condition icon
  3. Forecast Screen
     - 3-day weather forecast
- Web-based configuration interface
- Secure, open-source implementation

## Hardware Requirements

- ESP8266-based WiFi Clock Display (commonly sold on AliExpress)
- USB to Serial adapter for initial flashing (if needed)
- Micro USB cable for power

## Software Requirements

- PlatformIO IDE (recommended) or Arduino IDE
- Required Libraries:
  - U8g2 (for display)
  - ArduinoJson
  - NTPClient

## Installation

1. Clone this repository
2. Open the project in PlatformIO
3. Configure your COM port in `platformio.ini` if needed
4. Build and upload the firmware to your device

## Initial Setup

1. Power on your device
2. The device will create a WiFi access point named "ESP_XXXXXX" (where XXXXXX is a unique identifier)
3. Connect to this network using your phone or computer
4. Open a web browser and navigate to `192.168.4.1`
5. Follow the on-screen instructions to connect the device to your WiFi network

## Configuration Website

Once connected to your WiFi network, you can access the device's settings page:

1. Find the device's IP address (check your router's DHCP client list)
2. Open a web browser and navigate to `http://<device-ip-address>/settings` (Note: Must use `http://`, HTTPS is not supported)
3. You'll see the main configuration page with the following options:

### Current Features

- WiFi Configuration
  - Configure WiFi credentials

- Time Settings
  - Timezone selection from dropdown list
  - **New**: 12/24 hour format selection

- Display Features
  - Three rotating screens:
    1. Time with day/night progression
    2. Current weather conditions
    3. 3-day forecast display

- Weather Settings
  - Location settings
  - Update frequency
  - **Required**: OpenWeatherMap API key (obtain from [openweathermap.org](https://openweathermap.org/))

### Planned Enhancements (TODOs)

- Display Settings
  - [ ] Brightness control (investigating hardware support)
  - [ ] Display orientation options
  - [x] Time format selection (12/24 hour)
  
- Weather Settings
  - [ ] Temperature unit toggle (°C/°F)

- Time Settings
  - [ ] Daylight saving time configuration
  - [ ] NTP server configuration

- System Features
  - [ ] Current connection status display
  - [ ] More customization options

## Known Issues

The following issues are currently being tracked and worked on:

1. **Sunset to Sunrise Progression**: The sunset to sunrise transition feature is currently not functioning as expected and is under investigation. The sunrise to sunset progression is working correctly.

## Contributing

This is a new hobby project aimed at providing an open-source alternative for these popular WiFi clock displays. The project is in its early stages and welcomes contributions! Please feel free to submit pull requests or open issues for any bugs or feature requests.

## Why Open Source?

This project was created to provide users with:
- Transparency in what code runs on their devices
- Control over their data and privacy
- Ability to customize and extend functionality
- Independence from potentially insecure closed-source firmware

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Disclaimer

This is a community project and is not affiliated with the original device manufacturers. Use this firmware at your own risk.

## Support

If you encounter any issues or need help, please:
1. Check the existing issues on GitHub
2. Create a new issue with detailed information about your problem
3. Include your device information and any relevant error messages

## Acknowledgments

Thanks to the open-source community for the libraries and tools that make this project possible. 