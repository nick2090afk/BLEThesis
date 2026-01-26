#include "BLEDevice.h"
#include "Arduino.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "WiFiClientSecure.h"
#include "ArduinoJson.h"
#include "secrets.h" // Include WiFi credentials and other secrets

// Coospo hw807 Band Service UUIDs
#define HEART_RATE_SERVICE_UUID     "0000180d-0000-1000-8000-00805f9b34fb"
#define HEART_RATE_MEASUREMENT_UUID   "00002a37-0000-1000-8000-00805f9b34fb"
#define BATTERY_SERVICE_UUID          "0000180f-0000-1000-8000-00805f9b34fb"
#define BATTERY_LEVEL_UUID            "00002a19-0000-1000-8000-00805f9b34fb"

// WiFi credentials
// const char *ssid = "yourWiFiname";            // Replace with your WiFi name
// const char *password = "yourWiFipassword";    // Replace with your WiFi password

// MQTT Broker settings
const char *mqtt_broker = "wearable.duckdns.org";
const char *mqtt_topic = "home/esp32/data";
// const char *mqtt_username = "yourMQTTusername";   // Replace with your MQTT username
// const char *mqtt_password = "yourMQTTpassword";   // Replace with your MQTT password
const int mqtt_port = 8883;

// WiFi and MQTT client initialization
WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);

// Global variables
BLEClient* pClient = nullptr;
BLERemoteService* pBatteryService = nullptr;
BLERemoteCharacteristic* pBatteryChar = nullptr;
BLERemoteService* pSensorService = nullptr;
BLERemoteCharacteristic* pSensorChar = nullptr;
BLEAdvertisedDevice* myDevice = nullptr;
unsigned long packetCounter = 0;

bool deviceConnected = false;
bool doConnect = false;
bool servicesInitialized = false;
bool isScanning = false;
std::string targetDeviceName = "COOSPO HW807";

// Data storage
int currentHR = 0;
int batteryLevel = 0;
unsigned long lastDataRequest = 0;
class MyAdvertisedDeviceCallbacks;
class MyClientCallback;
MyAdvertisedDeviceCallbacks* pScanCallback = nullptr;
MyClientCallback* pClientCallback = nullptr;

// Root CA Certificate
const char *ca_cert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIEVzCCAj+gAwIBAgIRAKp18eYrjwoiCWbTi7/UuqEwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw
WhcNMjcwMzEyMjM1OTU5WjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg
RW5jcnlwdDELMAkGA1UEAxMCRTcwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAARB6AST
CFh/vjcwDMCgQer+VtqEkz7JANurZxLP+U9TCeioL6sp5Z8VRvRbYk4P1INBmbef
QHJFHCxcSjKmwtvGBWpl/9ra8HW0QDsUaJW2qOJqceJ0ZVFT3hbUHifBM/2jgfgw
gfUwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQGCCsGAQUFBwMCBggrBgEFBQcD
ATASBgNVHRMBAf8ECDAGAQH/AgEAMB0GA1UdDgQWBBSuSJ7chx1EoG/aouVgdAR4
wpwAgDAfBgNVHSMEGDAWgBR5tFnme7bl5AFzgAiIyBpY9umbbjAyBggrBgEFBQcB
AQQmMCQwIgYIKwYBBQUHMAKGFmh0dHA6Ly94MS5pLmxlbmNyLm9yZy8wEwYDVR0g
BAwwCjAIBgZngQwBAgEwJwYDVR0fBCAwHjAcoBqgGIYWaHR0cDovL3gxLmMubGVu
Y3Iub3JnLzANBgkqhkiG9w0BAQsFAAOCAgEAjx66fDdLk5ywFn3CzA1w1qfylHUD
aEf0QZpXcJseddJGSfbUUOvbNR9N/QQ16K1lXl4VFyhmGXDT5Kdfcr0RvIIVrNxF
h4lqHtRRCP6RBRstqbZ2zURgqakn/Xip0iaQL0IdfHBZr396FgknniRYFckKORPG
yM3QKnd66gtMst8I5nkRQlAg/Jb+Gc3egIvuGKWboE1G89NTsN9LTDD3PLj0dUMr
OIuqVjLB8pEC6yk9enrlrqjXQgkLEYhXzq7dLafv5Vkig6Gl0nuuqjqfp0Q1bi1o
yVNAlXe6aUXw92CcghC9bNsKEO1+M52YY5+ofIXlS/SEQbvVYYBLZ5yeiglV6t3S
M6H+vTG0aP9YHzLn/KVOHzGQfXDP7qM5tkf+7diZe7o2fw6O7IvN6fsQXEQQj8TJ
UXJxv2/uJhcuy/tSDgXwHM8Uk34WNbRT7zGTGkQRX0gsbjAea/jYAoWv0ZvQRwpq
Pe79D/i7Cep8qWnA+7AE/3B3S/3dEEYmc0lpe1366A/6GEgk3ktr9PEoQrLChs6I
tu3wnNLB2euC8IKGLQFpGtOO/2/hiAKjyajaBP25w1jF0Wl8Bbqne3uZ2q1GyPFJ
YRmT7/OXpmOH/FVLtwS+8ng1cAmpCujPwteJZNcDG0sF2n/sc0+SQf49fdyUK0ty
+VUwFj9tmWxyR/M=
-----END CERTIFICATE-----
)EOF";

// Forward declarations
void startScan();
bool initializeServices();
void scanCompleteCallback(BLEScanResults scanResults);
void connectToWiFi();
void connectToMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length); 

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) override {
    Serial.println("Connected to Coospo Armband!");
    deviceConnected = true;
    servicesInitialized = false;
  }
  
  void onDisconnect(BLEClient* pclient) override {
    Serial.println("Disconnected from Coospo Armband");
    deviceConnected = false;
    doConnect = false;
    servicesInitialized = false;
    
    // Reset service pointers
    pBatteryService = nullptr;
    pBatteryChar = nullptr;
    pSensorService = nullptr;
    pSensorChar = nullptr;
    
  }
};

// HR notification callback
void HRNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                      uint8_t* pData, size_t length, bool isNotify) {
  if (length > 1) {
    uint8_t flags = pData[0];
    bool isHRValue16bit = (flags & 0x01);
    if (isHRValue16bit && length > 2) {
        currentHR = (pData[2] << 8) | pData[1];
    } else {
        currentHR = pData[1];
    }
    Serial.printf("Heart Rate: %d bpm\n", currentHR);
  }
}

// Initialize BLE services
bool initializeServices() {
  Serial.println("Initializing Coospo services...");
  
  if (!pClient || !pClient->isConnected()) {
    Serial.println("Client not connected!");
    return false;
  }

  // Get Battery Service
  pBatteryService = pClient->getService(BATTERY_SERVICE_UUID);
  if (pBatteryService != nullptr) {
    Serial.println("Battery service found");
    pBatteryChar = pBatteryService->getCharacteristic(BATTERY_LEVEL_UUID);
    if (pBatteryChar != nullptr) {
      Serial.println("Battery characteristic found");
    }
  } else {
    Serial.println("Battery service not found");
  }

  // Get Heart Rate Service
  pSensorService = pClient->getService(HEART_RATE_SERVICE_UUID);
  if (pSensorService != nullptr) {
    Serial.println("Heart Rate service found");
    pSensorChar = pSensorService->getCharacteristic(HEART_RATE_MEASUREMENT_UUID);
    if (pSensorChar != nullptr) {
      Serial.println("Heart Rate characteristic found");
      
      // Register for notifications
      if (pSensorChar->canNotify()) {
        pSensorChar->registerForNotify(HRNotifyCallback);
        
        // IMPORTANT: Write to CCCD to enable notifications
        const uint8_t notificationOn[] = {0x1, 0x0};
        const uint8_t notificationOff[] = {0x0, 0x0};
        BLERemoteDescriptor* pNotifyDescriptor = pSensorChar->getDescriptor(BLEUUID((uint16_t)0x2902));
        if (pNotifyDescriptor) {
            pNotifyDescriptor->writeValue((uint8_t*)notificationOn, 2, true);
            Serial.println("Notifications enabled for HR");
        } else {
            Serial.println("Failed to find CCCD descriptor");
        }
      }
    } else {
      Serial.println("Heart Rate characteristic not found");
    }
  } else {
    Serial.println("Heart Rate service not found");
  }
  servicesInitialized = true;
  return (pSensorChar != nullptr || pBatteryChar != nullptr);
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // Check if this is our target device
    if (advertisedDevice.haveName() && advertisedDevice.getName() == targetDeviceName) {
      Serial.printf("Found target: %s\n", advertisedDevice.getName().c_str());
      BLEDevice::getScan()->stop();
      isScanning = false;  // We found our device, so scan is done
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

void scanCompleteCallback(BLEScanResults scanResults) {
  Serial.println("Scan finished.");
  BLEDevice::getScan()->clearResults();  // Free scan result memory
  isScanning = false;
}

void startScan() {
  if (isScanning) return;
  
  isScanning = true;
  Serial.println("Starting BLE scan...");

  // Clean up previous device reference
  if (myDevice != nullptr) {
    delete myDevice;
    myDevice = nullptr;
  }
  
  BLEScan* pBLEScan = BLEDevice::getScan();
  if (pScanCallback == nullptr) {
    pScanCallback = new MyAdvertisedDeviceCallbacks();
  }
  pBLEScan->setAdvertisedDeviceCallbacks(pScanCallback);
  
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  
  // Start scan with completion callback
  pBLEScan->start(10, scanCompleteCallback, false);
}

bool connectToServer() {
  Serial.print("Connecting to: ");
  Serial.println(myDevice->getAddress().toString().c_str());
  
  // Delete old client if connection previously failed
  if (pClient != nullptr && !pClient->isConnected()) {
    delete pClient;
    pClient = nullptr;
  }

  // Create new client if needed
  if (pClient == nullptr) {
    pClient = BLEDevice::createClient();
    if (pClientCallback == nullptr) {
      pClientCallback = new MyClientCallback();
    }
    pClient->setClientCallbacks(pClientCallback);
    Serial.println("Client created");
  }

  // Connect to remote BLE Server
  if (!pClient->connect(myDevice)) {
    Serial.println("Failed to connect");
    return false;
  }
  
  Serial.println("Connected to server");
  return true;
}

void requestSmartBandData() {
  if (pBatteryChar != nullptr && pBatteryChar->canRead()) {
    std::string value = pBatteryChar->readValue();
    if (!value.empty()) {
      batteryLevel = (uint8_t)value[0];
      Serial.printf("Battery Level: %d%%\n", batteryLevel);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize BLE first and let it stabilize
  BLEDevice::init("ESP32_BLE_Client");
  startScan();
  delay(2000);  // Allow BLE stack to stabilize
  
  // Then initialize WiFi/MQTT
  connectToWiFi();
  esp_client.setCACert(ca_cert);
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setKeepAlive(60);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTT();
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void connectToMQTT() {
  if (!mqtt_client.connected()) {
    String client_id = "esp32-client-" + String(WiFi.macAddress());
    Serial.printf("Attempting MQTT connection as %s...\n", client_id.c_str());
    
    if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker");
      mqtt_client.subscribe(mqtt_topic);
    } else {
      Serial.print("Failed, rc=");
      Serial.println(mqtt_client.state());
      // Will retry on next loop iteration
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }
  Serial.println("\n-----------------------");
}

void loop() {
  // Handle connection request
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Connection successful");
      doConnect = false;
    } else {
      Serial.println("Connection failed");
      doConnect = false;
      startScan();
    }
  }

  // Ensure MQTT connection
  static unsigned long lastMqttAttempt = 0;
 if (!mqtt_client.connected()) {
  if (millis() - lastMqttAttempt > 5000) {
    connectToMQTT();
    lastMqttAttempt = millis();
  }
} else {
  mqtt_client.loop();
}
  // Initialize services AFTER connection is established
  if (deviceConnected && !servicesInitialized) {
    delay(1000);
    if (initializeServices()) {
      Serial.println("Services initialized successfully");
    } else {
      Serial.println("Failed to initialize services");
    }
  }

  // Handle data requests when fully connected and initialized
if (deviceConnected && servicesInitialized && pClient != nullptr) {
  static unsigned long lastPublish = 0;
  
  if (millis() - lastPublish > 10000) {
    requestSmartBandData();
    
    // Publish after reading new data
    StaticJsonDocument<200> doc;
    doc["heart_rate"] = currentHR;
    doc["battery_level"] = batteryLevel;
    doc["seq_id"] = packetCounter++;
    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);
    
    if (mqtt_client.publish(mqtt_topic, jsonBuffer)) {
      Serial.println("Published JSON sensor data");
    } else {
      Serial.println("Publish failed!");
    }
    
    lastPublish = millis();
  }

  // Check connection status
  if (!pClient->isConnected()) {
    Serial.println("Connection lost!");
    deviceConnected = false;
    servicesInitialized = false;
    doConnect = false;
  }
}

  // If not connected and not trying to connect, restart scan
  if (!deviceConnected && !doConnect) {
    if (!isScanning) {
       Serial.println("Restarting scan...");
       delay(5000); // Wait 5 seconds before retrying
       startScan();
    }
  }
  
  delay(100);
}