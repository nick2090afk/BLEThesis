# BLE Heart Rate Monitor Client - Coospo HW807 Integration

ESP32-WROOM BLE client that connects to the Coospo HW807 commercial heart rate monitor armband, receives real-time heart rate and battery level data, and publishes it to a secure MQTT broker. Developed using VS Code with PlatformIO for professional embedded development workflows.

## Hardware Requirements

### Required Components
- **ESP32-WROOM** development board (any variant)
- **Coospo HW807** Heart Rate Monitor Armband (or compatible BLE HR device)
- USB cable for programming
- Stable WiFi network
- MQTT broker with TLS support (local or cloud)

## Software Requirements

### Development Environment
- **Visual Studio Code** (latest version)
- **PlatformIO IDE Extension** for VS Code
- **Git** (optional, for version control)
Installation
Install VS Code and PlatformIO

    Download and install Visual Studio Code from https://code.visualstudio.com/

    Open VS Code and go to Extensions (Ctrl+Shift+X)

    Search for "PlatformIO IDE" and click Install

    Restart VS Code after installation

## Project Setup

    Clone or download this repository

    Open VS Code

    Click "Open Folder" and select the project directory

    PlatformIO will automatically detect the project and install dependencies

## Project Structure

    project_root/
    ├── .vscode
    ├── include
    ├── lib
    ├── src/
    │   └── main.cpp          # Main source code
    │   └── secrets.h         # WiFi and MQTT credentials
    ├── test
    ├── platformio.ini        # PlatformIO configuration
    └── README.md

## Configuration platformio.ini

### Your platformio.ini file should contain:

    text
    [env:esp32dev]
    platform = espressif32
    board = esp32dev
    framework = arduino
    monitor_speed = 115200
    board_build.partitions = min_spiffs.csv
    lib_deps = knolleary/PubSubClient@^2.8, bblanchon/ArduinoJson@^6.18.5

### Adjust the board parameter if you're using a different ESP32 board variant.

## Create secrets.h File
### Create a file named secrets.h in the include/ directory with the following content:

    cpp
    #pragma once

    // WiFi credentials
    const char *ssid = "your_wifi_name";
    const char *password = "your_wifi_password";

    // MQTT credentials
    const char *mqtt_username = "your_mqtt_username";
    const char *mqtt_password = "your_mqtt_password";


### Replace the placeholder values with your actual credentials.
### Update Certificate

## If your MQTT broker uses a different SSL certificate, replace the ca_cert constant in main.cpp with your broker's root CA certificate.
Modify Device Name (Optional)

If you're using a different BLE heart rate monitor, change the target device name in main.cpp:

    cpp
    std::string targetDeviceName = "COOSPO HW807";

## Usage
### Build the Project

    Open the project in VS Code

    Click the checkmark icon in the PlatformIO toolbar (or press Ctrl+Alt+B)

    Wait for the build to complete

### Upload to ESP32

    Connect your ESP32 to your computer via USB

    Click the right arrow icon in the PlatformIO toolbar (or press Ctrl+Alt+U)

    PlatformIO will automatically detect the port and upload

### Monitor Serial Output

    Click the plug icon in the PlatformIO toolbar (or press Ctrl+Alt+S)

    Serial monitor will open at 115200 baud showing:

        BLE scanning progress

        Connection status

        Heart rate readings

        Battery levels

        MQTT publish confirmations

## PlatformIO Commands

### Access the PlatformIO menu by clicking the alien icon in the left sidebar:

    Build: Compile the project

    Upload: Upload firmware to ESP32

    Upload and Monitor: Upload and immediately open serial monitor

    Clean: Remove build files

    Test: Run unit tests (if configured)

## Data Format

### The device publishes JSON data to the MQTT topic every 10 seconds:

#### Examble:

    json
    {
       "heart_rate": 75,
       "battery_level": 85
    }

### Configuration Details
#### BLE Settings (Find your device UUIDs using nRF connect for android: https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp&hl=en and for iOS: https://apps.apple.com/us/app/nrf-connect-for-mobile/id1054362403)

    Service UUID: 0000180d-0000-1000-8000-00805f9b34fb (Heart Rate)

    Characteristic UUID: 00002a37-0000-1000-8000-00805f9b34fb (Heart Rate Measurement)

    Battery Service UUID: 0000180f-0000-1000-8000-00805f9b34fb

    Battery Characteristic UUID: 00002a19-0000-1000-8000-00805f9b34fb

## MQTT Settings

    Broker: add your MQTT broker

    Port: 8883 (SSL/TLS)

    Topic: add your topic

    Keep-alive: 60 seconds

## Timing

    BLE scan duration: 10 seconds

    Data publish interval: 10 seconds

    MQTT reconnect attempt interval: 5 seconds

    Scan restart delay: 5 seconds

## Troubleshooting
### PlatformIO Cannot Find Board

    Check that your ESP32 is connected via USB

    Verify the correct board is specified in platformio.ini

    Try a different USB cable or port

    Install USB drivers if necessary (CP2102 or CH340 depending on your board)

### Build Errors

    Clean the project: PlatformIO > Clean

    Delete the .pio folder and rebuild

    Check that all libraries are installed correctly

    Verify secrets.h is in the include/ directory

### Cannot Find BLE Device

    Ensure the heart rate monitor is powered on and in pairing mode

    Check that the device name matches your monitor

    Verify the monitor is not connected to another device

### WiFi Connection Issues

    Verify SSID and password in secrets.h

    Check WiFi signal strength

    Ensure your network supports 2.4 GHz (ESP32 does not support 5 GHz)

### MQTT Connection Failures

    Verify MQTT broker address and port

    Check username and password

    Ensure the certificate matches your broker

    Confirm the broker is accessible from your network

### No Heart Rate Data

    Check serial monitor for "Notifications enabled for HR" message

    Ensure the heart rate monitor is worn properly

    Verify the monitor is detecting heart rate (check monitor's own display)

### Upload Failed

    Hold the BOOT button on ESP32 while uploading

    Reset the board and try again

    Check that no other program is using the serial port

    Verify correct upload speed in platformio.ini (default is usually fine)


## Notes

    The ESP32 connects to both BLE and WiFi simultaneously

    Heart rate data is received via BLE notifications (no polling required)

    Battery level is read on demand every 10 seconds

    The device automatically reconnects if either BLE or MQTT connection is lost

    MQTT messages use QoS 0 (fire and forget)

    PlatformIO automatically manages library dependencies and board configurations

## MQTT settings

    MQTT messages use QoS 0 (fire and forget)
    
### For QoS 1 modify the code: 
    
    mqtt_client.publish(mqtt_topic, jsonBuffer, false); // QoS 0 (current) 
    mqtt_client.subscribe(mqtt_topic, 1); // QoS 1 for subscribe

## Medical Device Disclaimer

!!!! IMPORTANT !!!!: This project is for academic and development purposes only.

    Not FDA approved or CE marked for medical use

    Not intended to diagnose, treat, or monitor medical conditions

    MAX30100 accuracy is limited compared to medical-grade devices

    Always consult qualified medical professionals for health monitoring

    Do not rely on this device for critical health decisions
    

## License

This project is released under the MIT License for academic and research purposes.
