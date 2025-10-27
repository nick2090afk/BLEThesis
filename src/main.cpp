#include "BLEDevice.h"
#include "Arduino.h"

// Coospo hw807 Band ACTUAL Service UUIDs (from reverse engineering)
#define HEART_RATE_SERVICE_UUID       "0000180d-0000-1000-8000-00805f9b34fb"
#define HEART_RATE_MEASUREMENT_UUID   "00002a37-0000-1000-8000-00805f9b34fb"
#define BATTERY_SERVICE_UUID          "0000180f-0000-1000-8000-00805f9b34fb"
#define BATTERY_LEVEL_UUID            "00002a19-0000-1000-8000-00805f9b34fb"

// Global variables for better state management
BLEClient* pClient = nullptr;
BLERemoteService* pService = nullptr;
BLERemoteService* pBatteryService = nullptr;
BLERemoteCharacteristic* pBatteryChar = nullptr;
BLERemoteService* pSensorService = nullptr;
BLERemoteCharacteristic* pSensorChar = nullptr;

bool deviceConnected = false;
bool doConnect = false;
std::string targetDeviceName = "COOSPO HW807";

// Data storage
int currentHR = 0;
int batteryLevel = 0;
unsigned long lastDataRequest = 0;

// Forward declaration for startScan
void startScan();

// Forward declaration for initializeServices
bool initializeServices();

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) override {
    Serial.println("Connected to Coospo Armband!");
    deviceConnected = true;
    
    // Initialize services after connection
    delay(1000); // Give device time to settle
    initializeServices();
  }
  
  void onDisconnect(BLEClient* pclient) override {
    Serial.println("Disconnected from Coospo Armband");
    deviceConnected = false;
    doConnect = false;
    
    // Reset service pointers
    pService = nullptr;
    pBatteryService = nullptr;
    pBatteryChar = nullptr;
    pSensorService = nullptr;
    pSensorChar  = nullptr;

    
    Serial.println("Restarting scan in 5 seconds...");
    delay(5000);
    startScan();
  }
};

// Forward declaration for initializeServices
bool initializeServices();

// HR notification callback
void HRNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                      uint8_t* pData, size_t length, bool isNotify) {
  // Byte 0: Flags, Byte 1: HR (if 8bit), Byte 1+2: HR (if 16bit)
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

// Initialize BLE services and characteristics
bool initializeServices() {
  Serial.println("Initializing Coospo services...");

  pBatteryService = pClient->getService(BATTERY_SERVICE_UUID);
  if (pBatteryService != nullptr) {
    pBatteryChar = pBatteryService->getCharacteristic(BATTERY_LEVEL_UUID);
    if (pBatteryChar != nullptr) {
      Serial.println("Battery characteristic found");
    }
  } else {
    Serial.println("Battery service not found");
  }

  pSensorService = pClient->getService(HEART_RATE_SERVICE_UUID);
  if (pSensorService != nullptr) {
    pSensorChar = pSensorService->getCharacteristic(HEART_RATE_MEASUREMENT_UUID);
    if (pSensorChar != nullptr) {
      Serial.println("Heart Rate characteristic found");
      
      pSensorChar->registerForNotify(HRNotifyCallback);
    } else {
      Serial.println("Heart Rate characteristic not found");
    }
  } else {
    Serial.println("Heart Rate service not found");
  }

  return (pSensorChar != nullptr || pBatteryChar != nullptr);
}


// Separate function for cleaner code organization
void startScan() {
  Serial.println("Starting BLE scan...");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);  // Set scan interval
  pBLEScan->setWindow(99);     // Set scan window
  
  BLEScanResults scanResults = pBLEScan->start(10, false); // 10 sec scan, don't delete results
  
  Serial.printf("Found %d devices\n", scanResults.getCount());
  
  for (int i = 0; i < scanResults.getCount(); i++) {
    BLEAdvertisedDevice device = scanResults.getDevice(i);
    
    // Better device identification - exact match for Coospo HW807
    if (device.haveName() && device.getName() == targetDeviceName) {
      
      Serial.printf("Target device found: %s (RSSI: %d)\n", 
                    device.getName().c_str(), device.getRSSI());
      
      // Create client if not already created
      if (pClient == nullptr) {
        pClient = BLEDevice::createClient();
        pClient->setClientCallbacks(new MyClientCallback());
      }
      
      // Attempt connection with error handling
      if (pClient->connect(&device)) {
        Serial.println("Connection initiated...");
        doConnect = true;
      } else {
        Serial.println("Failed to connect to device");
      }
      break;
    }
  }
  
  // Clean up scan results
  pBLEScan->clearResults();
  
  if (!doConnect) {
    Serial.println("Target device not found, retrying in 10 seconds...");
  }
}

// Forward declaration for requestSmartBandData
void requestSmartBandData();

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 BLE Smartwatch Client Starting...");
  
  // Initialize BLE with error checking
  BLEDevice::init("ESP32_BLE_Client");
  Serial.println("BLE Device initialized");
  
  // Start initial scan
  startScan();
}

void loop() {
  // Connection management
  if (doConnect && !deviceConnected) {
    delay(1000); // Give connection time to establish
  }
  
  // Handle ongoing communication when connected
  if (deviceConnected && pClient != nullptr) {
    // Request data every 10 seconds
    if (millis() - lastDataRequest > 10000) {
      requestSmartBandData();
      lastDataRequest = millis();
    }
    
    // Keep connection alive
    if (!pClient->isConnected()) {
      deviceConnected = false;
      Serial.println("Connection lost unexpectedly");
    }
  }
  
  // Retry connection if not connected and not currently trying
  if (!deviceConnected && !doConnect) {
    delay(10000); // Wait 10 seconds before retry
    startScan();
  }
  
  delay(1000); // Main loop delay
}

// Request data from Coospo
void requestSmartBandData() {
  // Read battery level
  if (pBatteryChar != nullptr && pBatteryChar->canRead()) {
    std::string batteryValue = pBatteryChar->readValue();
    if (!batteryValue.empty()) {
      batteryLevel = (uint8_t)batteryValue[0];
      Serial.printf("Battery Level: %d%%\n", batteryLevel);
    }
  }
  
  // Enable realtime steps if available
 
  
  // Print summary
  Serial.printf("Status - HR: %d, Battery: %d%%\n", 
                currentHR, batteryLevel);
}

// Optional: Add cleanup function
void cleanup() {
  if (pClient != nullptr) {
    if (pClient->isConnected()) {
      pClient->disconnect();
    }
    delete pClient;
    pClient = nullptr;
  }
}