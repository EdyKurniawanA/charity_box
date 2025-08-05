# Kotak Amal Nurul Ilmi - IoT Security System

## Overview
This is an IoT security system project using Arduino Mega, ESP32 Cam, Telegram Bot, and various sensors. The system provides fingerprint authentication, GPS tracking, live camera streaming, and remote monitoring capabilities.

## Components Used
- **Arduino Mega 2560** - Main controller
- **ESP32 Cam** - Camera module for live streaming
- **Fingerprint Sensor (R307)** - Biometric authentication
- **GPS NEO-6M** - Location tracking
- **LCD I2C 16x2** - Status display
- **Vibration Sensor** - Motion detection
- **Buzzer** - Audio alerts
- **Relay Module** - Door lock control
- **SIM800L** - Backup SMS communication

## Features
1. **Islamic Greeting** - LCD displays "Assalamualaikum, User" on startup
2. **Fingerprint Authentication** - Secure access control with Telegram notifications
3. **GPS Tracking** - Remote location monitoring via Telegram
4. **Hybrid Camera System** - ESP32 Cam with Web interface + Telegram control
5. **Vibration Detection** - Motion alerts sent to Telegram
6. **Status Monitoring** - Real-time system status updates

## File Structure
```
├── sketch_jun30c/
│   └── sketch_jun30c.ino          # Main Arduino Mega program
├── CameraWebServer/
│   ├── CameraWebServer.ino         # Hybrid ESP32 Cam program (Web + Telegram)
│   ├── app_httpd.cpp               # Web server implementation
│   ├── camera_pins.h               # Camera pin definitions
│   └── camera_index.h              # Web interface HTML
├── config/
│   └── telegram_config.h           # Telegram Bot configuration
├── gps/
│   └── gps.ino                     # GPS testing program (legacy)
├── sim800l_v2/
│   └── sim800l_v2.ino             # SIM800L testing program (legacy)
└── README.md                       # This file
```

## Setup Instructions

### 1. Hardware Connections

#### Arduino Mega Connections:
- **GPS Module**: TX → Pin 17, RX → Pin 16
- **Fingerprint Sensor**: TX → Pin 19, RX → Pin 18
- **LCD I2C**: SDA → Pin 20, SCL → Pin 21
- **Buzzer**: Pin 5
- **Relay**: Pin 4
- **Vibration Sensor**: A0
- **ESP32 Cam Enable**: Pin 3

#### ESP32 Cam Connections:
- **Camera**: Built-in OV2640 sensor
- **WiFi**: Built-in WiFi module
- **Control**: Receives commands from Arduino Mega via GPIO

### 2. Software Setup

#### Required Libraries:
```cpp
// Arduino Mega
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ESP32 Cam
#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
```

#### Installation:
1. Open Arduino IDE
2. Go to Tools → Manage Libraries
3. Install the following libraries:
   - LiquidCrystal_I2C
   - TinyGPS++
   - Adafruit Fingerprint Sensor
   - ArduinoJson

### 3. Telegram Bot Setup

1. **Create a Telegram Bot**:
   - Message @BotFather on Telegram
   - Send `/newbot`
   - Follow instructions to create your bot
   - Save the bot token

2. **Get Chat ID**:
   - Message your bot
   - Visit: `https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates`
   - Find your chat ID in the response

3. **Update Configuration**:
   - Edit `config/telegram_config.h`
   - Replace placeholder values with your actual credentials

### 4. WiFi Configuration

Update the WiFi credentials in both programs:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### 5. Upload Programs

1. **Arduino Mega**:
   - Connect Arduino Mega to computer
   - Open `sketch_jun30c/sketch_jun30c.ino`
   - Select board: Arduino Mega 2560
   - Upload the program

2. **ESP32 Cam**:
   - Connect ESP32 Cam to computer
   - Open `CameraWebServer/CameraWebServer.ino`
   - Select board: ESP32 Dev Module
   - Upload the program

## Telegram Bot Commands

### Main System Commands:
- `/camera` - Toggle ESP32 Cam streaming
- `/gps` - Get current GPS location
- `/status` - Get system status
- `/door` - Manually toggle door lock (emergency access)

### ESP32 Cam Commands (Telegram):
- `/capture` - Take a photo and send to Telegram
- `/status` - Get camera status via Telegram
- `/stream_start` - Start streaming monitoring
- `/stream_stop` - Stop streaming monitoring
- `/help` - Show available commands

### ESP32 Cam Web Interface:
- **http://[ESP32-IP]** - Main camera interface
- **http://[ESP32-IP]/stream** - Live video stream
- **http://[ESP32-IP]/capture** - Take a photo
- **http://[ESP32-IP]/status** - Camera status
- **http://[ESP32-IP]/control** - Camera controls (brightness, contrast, etc.)

## System Flow

1. **Startup**:
   - LCD displays "Assalamualaikum, User"
   - Checks fingerprint sensor status
   - Connects to WiFi
   - Sends startup message to Telegram

2. **Fingerprint Authentication**:
   - User places finger on sensor
   - System validates fingerprint
   - Sends result to Telegram
   - Controls relay (door lock)
   - **Registered User**: Door unlocks for 3 seconds, then locks automatically
   - **Unregistered User**: Door remains locked, access denied

3. **Remote Control**:
   - User sends commands via Telegram
   - System executes commands
   - Sends confirmation messages

4. **Monitoring**:
   - Vibration detection
   - GPS tracking
   - Status updates
   - Live camera streaming

## Troubleshooting

### Common Issues:

1. **WiFi Connection Failed**:
   - Check WiFi credentials
   - Ensure strong signal
   - Restart system

2. **Fingerprint Not Working**:
   - Clean sensor surface
   - Re-enroll fingerprints
   - Check wiring connections

3. **GPS No Signal**:
   - Move to open area
   - Wait for satellite lock
   - Check antenna connection

4. **Telegram Messages Not Sending**:
   - Verify bot token and chat ID
   - Check internet connection
   - Ensure bot is not blocked

### Debug Information:
- Serial Monitor shows detailed debug information
- Check Serial Monitor for error messages
- Use `/status` command to check system health

## Security Features

1. **Biometric Authentication** - Secure fingerprint access
2. **Remote Monitoring** - Real-time status updates
3. **Motion Detection** - Vibration sensor alerts
4. **Location Tracking** - GPS coordinates
5. **Live Video** - ESP32 Cam streaming
6. **Backup Communication** - SIM800L SMS fallback

## Customization

### Adding New Commands:
1. Add command definition in `telegram_config.h`
2. Add command handler in main program
3. Update help message

### Modifying Messages:
- Edit message constants in `telegram_config.h`
- Update LCD display messages in main program

### Adding New Sensors:
1. Define pin connections
2. Add sensor library
3. Create sensor functions
4. Integrate with Telegram notifications

## Support

For technical support or questions, please refer to the Arduino and ESP32 documentation, or contact the development team.

## License

This project is developed for educational and charitable purposes. Please respect the Islamic values and principles in its use. 