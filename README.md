# WiFi Clock - Open Source Alternative

This is an open-source project that provides alternative firmware for the ESP8266-based WiFi Clock display commonly found on AliExpress ([like this one](https://www.aliexpress.us/item/3256806715737170.html)). This project aims to provide users with a transparent, customizable, and secure alternative to the original closed-source Chinese software.

## Features

- Easy WiFi configuration through a web interface
- Real-time clock synchronization using NTP
- Weather display
- Customizable display settings
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
2. The device will create a WiFi access point named "WiFi Clock Setup"
3. Connect to this network using your phone or computer
4. Open a web browser and navigate to `192.168.4.1`
5. Follow the on-screen instructions to connect the device to your WiFi network

## Configuration Website

Once connected to your WiFi network, you can access the device's settings page:

1. Find the device's IP address (check your router's DHCP client list)
2. Open a web browser and enter the IP address
3. You'll see the main configuration page with the following options:

### Available Settings

- WiFi Configuration
  - View current connection status
  - Change WiFi credentials if needed
  
- Display Settings
  - Brightness control
  - Display orientation
  - Time format (12/24 hour)
  
- Weather Settings
  - Location settings
  - Update frequency
  - Temperature unit (°C/°F)

- Time Settings
  - Time zone selection
  - NTP server configuration
  - Daylight saving time options

## Contributing

This is a hobby project aimed at providing an open-source alternative for these popular WiFi clock displays. Contributions are welcome! Please feel free to submit pull requests or open issues for any bugs or feature requests.

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

Thanks to all contributors and the open-source community for making this project possible. 
