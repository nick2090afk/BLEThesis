#include "BLEDevice.h"
#include "Arduino.h"

// Coospo hw807 Band Service UUIDs
#define HEART_RATE_SERVICE_UUID     "0000180d-0000-1000-8000-00805f9b34fb"
#define HEART_RATE_MEASUREMENT_UUID   "00002a37-0000-1000-8000-00805f9b34fb"
#define BATTERY_SERVICE_UUID          "0000180f-0000-1000-8000-00805f9b34fb"
#define BATTERY_LEVEL_UUID            "00002a19-0000-1000-8000-00805f9b34fb"

// Global variables
BLEClient* pClient = nullptr;
BLERemoteService* pBatteryService = nullptr;
BLERemoteCharacteristic* pBatteryChar = nullptr;
BLERemoteService* pSensorService = nullptr;
BLERemoteCharacteristic* pSensorChar = nullptr;
BLEAdvertisedDevice* myDevice = nullptr;

bool deviceConnected = false;
bool doConnect = false;
bool servicesInitialized = false;
bool isScanning = false; // Flag to track scan state
std::string targetDeviceName = "COOSPO HW807";

// Data storage
int currentHR = 0;
int batteryLevel = 0;
unsigned long lastDataRequest = 0;

// Forward declarations
void startScan();
bool initializeServices();
void scanCompleteCallback(BLEScanResults scanResults); 

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

// ADDED: This function is called when the scan (set in startScan) completes
void scanCompleteCallback(BLEScanResults scanResults) {
  Serial.println("Scan finished.");
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
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  
  // UPDATED: Start scan with a 10-second duration and a completion callback
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
    pClient->setClientCallbacks(new MyClientCallback());
    Serial.println("Client created");
  }

  // Connect to remote BLE Server
  if (!pClient->connect(myDevice)) {
    Serial.println("Failed to connect");
    // Don't delete client here, just return false
    return false;
  }
  
  Serial.println("Connected to server");
  return true;
}

void requestSmartBandData() {
  // Read battery level
  if (pBatteryChar != nullptr && pBatteryChar->canRead()) {
    std::string batteryValue = pBatteryChar->readValue();
    if (!batteryValue.empty()) {
      batteryLevel = (uint8_t)batteryValue[0];
      Serial.printf("Battery Level: %d%%\n", batteryLevel);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 BLE Coospo Client Starting...");
  
  BLEDevice::init("ESP32_BLE_Client");
  Serial.println("BLE Device initialized");
  
  startScan();
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

  // Initialize services AFTER connection is established
  if (deviceConnected && !servicesInitialized) {
    delay(1000);  // Give connection time to stabilize
    if (initializeServices()) {
      Serial.println("Services initialized successfully");
    } else {
      Serial.println("Failed to initialize services");
      // Optional: disconnect to force a full rescan cycle
      // if (pClient) pClient->disconnect();
    }
  }

  // Handle data requests when fully connected and initialized
  if (deviceConnected && servicesInitialized && pClient != nullptr) {
    if (millis() - lastDataRequest > 10000) {
      requestSmartBandData();
      lastDataRequest = millis();
    }
    
    // Check connection status (fallback)
    if (!pClient->isConnected()) {
      Serial.println("Connection lost (detected in loop)");
      deviceConnected = false;
      servicesInitialized = false;
      doConnect = false;
    }
  }
  
  if (!deviceConnected && !doConnect) {
    if (!isScanning) {
       Serial.println("Loop: Restarting scan...");
       delay(5000); // Wait 5 seconds before retrying
       startScan();
    }
  }
  
  delay(100);
}