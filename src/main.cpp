#include "BLEDevice.h"
#include "Arduino.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "WiFiClientSecure.h"
#include "ArduinoJson.h"
#include "secrets.h" 

// UUIDs for Coospo
#define HR_SERVICE_UUID "0000180d-0000-1000-8000-00805f9b34fb"
#define HR_CHAR_UUID "00002a37-0000-1000-8000-00805f9b34fb"
#define BAT_SERVICE_UUID "0000180f-0000-1000-8000-00805f9b34fb"
#define BAT_LEVEL_UUID "00002a19-0000-1000-8000-00805f9b34fb"

WiFiClientSecure espClient;
PubSubClient client(espClient);

// BLE globals
BLEClient* bleClient = nullptr;
BLERemoteCharacteristic* batChar = nullptr;
BLERemoteCharacteristic* hrChar = nullptr;
BLEAdvertisedDevice* myDevice = nullptr;

bool connected = false;
bool doConnect = false;
bool scanning = false;
String targetName = "COOSPO HW807";

int hrValue = 0;
int batLevel = 0;
unsigned long lastMsg = 0;
unsigned long count = 0;


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

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    connected = true;
    Serial.println("BLE connected");
  }
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("BLE disconnected");
  }
};

// handle notifications
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    // byte 0 is flags, byte 1 is value if 8bit
    if(length > 1) {
       hrValue = pData[1]; // assume 8bit for now
       Serial.printf("Got HR: %d\n", hrValue);
    }
}

bool setupServices() {
  if (!bleClient) return false;

  // Battery
  BLERemoteService* pRemoteService = bleClient->getService(BAT_SERVICE_UUID);
  if (pRemoteService) {
      batChar = pRemoteService->getCharacteristic(BAT_LEVEL_UUID);
  }

  // HR
  pRemoteService = bleClient->getService(HR_SERVICE_UUID);
  if (pRemoteService) {
      hrChar = pRemoteService->getCharacteristic(HR_CHAR_UUID);
      if(hrChar && hrChar->canNotify()) {
          hrChar->registerForNotify(notifyCallback);
          
          // enable notifications
          const uint8_t on[] = {0x1, 0x0};
          hrChar->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)on, 2, true);
      }
  }
  
  return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Device: "); 
    Serial.println(advertisedDevice.toString().c_str());
    
    if (advertisedDevice.getName() == targetName.c_str()) {
      Serial.println("Target found!");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      scanning = false;
    }
  }
};

// Callback for when the scan finishes
void scanCompleteCB(BLEScanResults scanResults) {
  Serial.println("Scan complete!");
  scanning = false; 
  // Clean up results to free memory
  BLEDevice::getScan()->clearResults();
}

void startScan() {
  scanning = true; 
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, scanCompleteCB, false);
}

void setup() {
  Serial.begin(115200);
  
  // Connect WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi OK");

  // Secure Client setup
  espClient.setCACert(ca_cert);
  client.setServer(mqtt_server, mqtt_port);
  
  // Start BLE
  BLEDevice::init("");
  startScan();
}

void loop() {
  if (!client.connected()) {
      if(client.connect("ESP32_Wearable", mqtt_username, mqtt_password)) {
          Serial.println("MQTT Connected");
      }
  }
  client.loop();

  if (doConnect) {
    if (bleClient == nullptr) {
        bleClient = BLEDevice::createClient();
        bleClient->setClientCallbacks(new MyClientCallback());
    }

    Serial.println("Forming a connection to the device...");
    
    // Attempt connection
    if(bleClient->connect(myDevice)) {
        Serial.println("Connected to server");
        setupServices();
    } else {
        Serial.println("Failed to connect to device");
    }

    delete myDevice;
    myDevice = nullptr;

    doConnect = false;
  }

  if (connected) {
      long now = millis();
      if (now - lastMsg > 1000) {
          lastMsg = now;
          
          // Read battery
          if(batChar) {
              std::string val = batChar->readValue();
              if(val.length() > 0) {
                  batLevel = (int)val[0];
              }
          }

          // JSON doc
          JsonDocument doc;
          doc["heart_rate"] = hrValue;
          doc["battery_level"] = batLevel;
          doc["seq_id"] = count++;
          
          char buffer[256];
          serializeJson(doc, buffer);
          client.publish(topic, buffer);
          Serial.println(buffer);
      }
  } 
  else {
     if(!doConnect && !scanning) {
          Serial.println("Device not found or disconnected. Starting new scan...");
          startScan();
  }
}
  delay(10);
}