ESP32 BLE Heart Rate Monitor with MQTT

A simple ESP32 application that connects to a Coospo HW807 heart rate armband via Bluetooth Low Energy (BLE) and publishes heart rate and battery data to an MQTT broker over WiFi.
Hardware Requirements

    ESP32 development board

    Coospo HW807 heart rate armband

    WiFi network access

    MQTT broker with SSL/TLS support

Software Requirements

    Arduino IDE or Vs Code with platformio plugin

    ESP32 board support package

    Required libraries:

        BLEDevice (ESP32 BLE Arduino)

        WiFi (ESP32 built-in)

        PubSubClient

        WiFiClientSecure

        ArduinoJson

Installation
Install Libraries

Open Arduino IDE and install the following libraries through Library Manager:

    PubSubClient by Nick O'Leary

    ArduinoJson by Benoit Blanchon

The BLE, WiFi, and WiFiClientSecure libraries are included with the ESP32 board package.
Install ESP32 Board Support

    Open Arduino IDE preferences

    Add this URL to Additional Board Manager URLs:

    text
    https://dl.espressif.com/dl/package_esp32_index.json

    Go to Tools > Board > Boards Manager

    Search for "ESP32" and install "ESP32 by Espressif Systems"

Configuration
Create secrets.h File

Create a file named secrets.h in the same directory as your main sketch with the following content:

cpp
#ifndef SECRETS_H
#define SECRETS_H

// WiFi credentials
const char *ssid = "your_wifi_name";
const char *password = "your_wifi_password";

// MQTT credentials
const char *mqtt_username = "your_mqtt_username";
const char *mqtt_password = "your_mqtt_password";

#endif

Replace the placeholder values with your actual credentials.
Update Certificate

If your MQTT broker uses a different SSL certificate, replace the ca_cert constant in the main code with your broker's root CA certificate.
Modify Device Name (Optional)

If you're using a different BLE heart rate monitor, change the target device name:

cpp
std::string targetDeviceName = "COOSPO HW807";

Usage
Upload the Code

    Connect your ESP32 to your computer via USB

    Select the correct board and port in Arduino IDE or VS code(platformio)

    Click Upload

Monitor Operation

Open the Serial Monitor at 115200 baud to view:

    BLE scanning progress

    Connection status

    Heart rate readings

    Battery levels

    MQTT publish confirmations

Data Format

The device publishes JSON data to the MQTT topic every 10 seconds:

json
{
  "heart_rate": 75,
  "battery_level": 85
}

Configuration Details
BLE Settings

    Service UUID: 0000180d-0000-1000-8000-00805f9b34fb (Heart Rate)

    Characteristic UUID: 00002a37-0000-1000-8000-00805f9b34fb (Heart Rate Measurement)

    Battery Service UUID: 0000180f-0000-1000-8000-00805f9b34fb

    Battery Characteristic UUID: 00002a19-0000-1000-8000-00805f9b34fb

MQTT Settings

    Broker: add your MQTT Broker

    Port: 8883 (SSL/TLS)

    Topic: add your MQTT topic

    Keep-alive: 60 seconds

Timing

    BLE scan duration: 10 seconds

    Data publish interval: 10 seconds

    MQTT reconnect attempt interval: 5 seconds

    Scan restart delay: 5 seconds

Troubleshooting
Cannot Find BLE Device

    Ensure the heart rate monitor is powered on and in pairing mode

    Check that the device name matches your monitor

    Verify the monitor is not connected to another device

WiFi Connection Issues

    Verify SSID and password in secrets.h

    Check WiFi signal strength

    Ensure your network supports 2.4 GHz (ESP32 does not support 5 GHz)

MQTT Connection Failures

    Verify MQTT broker address and port

    Check username and password

    Ensure the certificate matches your broker

    Confirm the broker is accessible from your network

No Heart Rate Data

    Check serial monitor for "Notifications enabled for HR" message

    Ensure the heart rate monitor is worn properly

    Verify the monitor is detecting heart rate (check monitor's own display)

Memory Issues

The code includes memory management features like clearing scan results to prevent heap fragmentation. If you experience crashes, monitor free heap in Serial Monitor.
Notes

    The ESP32 connects to both BLE and WiFi simultaneously

    Heart rate data is received via BLE notifications (no polling required)

    Battery level is read on demand every 10 seconds

    The device automatically reconnects if either BLE or MQTT connection is lost

    MQTT messages use QoS 0 (fire and forget)
    For QoS 1 modify the code: 
    mqtt_client.publish(mqtt_topic, jsonBuffer, false); // QoS 0 (current) 
    mqtt_client.subscribe(mqtt_topic, 1); // QoS 1 for subscribe
    
If your ESP32 microcontroller has 2MB of memory you need to add 

License

This code is provided as-is for educational and personal use.
