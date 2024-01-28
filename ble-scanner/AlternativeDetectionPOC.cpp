#include <Arduino.h>
#include <BLEScan.h>
#include <BLEDevice.h>
#define MAX_DEVICES 20  // Adjust the maximum number of devices to store
int attackThreshhold = 10; // Adjust the number of repeated service captures to trigger a positive 

struct BLEDeviceInfo {
  String serviceUUID;
  int count;
  int rssi;
};

BLEDeviceInfo knownServices[MAX_DEVICES];
int knownServicesCount = 0;

void setup() {
  Serial.begin(115200);
  // Start BLE scan
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void loop() {
  BLEScanResults foundDevices = BLEDevice::getScan()->start(5, false);
  memset(knownServices, 0, sizeof(knownServices));
  for (int i = 0; i < foundDevices.getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices.getDevice(i);
    // for each device check the service
    String uuid = device.getServiceUUID().toString().c_str();
    int rssi = device.getRSSI();
    // Check if the service is already known
    bool isServiceNew = true;
    for (int j = 0; j < knownServicesCount; j++) {
      if (knownServices[j].serviceUUID == uuid) {
        isServiceNew = false;
        knownServices[j].count++;
        knownServices[j].rssi = rssi;
        // if the uuid has been used by enough devices, they are probably fake! this could be an attack!
        if(uuid != "<NULL>"){
          if(knownServices[j].count == attackThreshhold){
            Serial.println("ATTACK DETECTED!!!!!!!!!!!!!!!!!");
            //TODO write method to call here, which prints hot/cold based on the rssi
          }
        }
        break;
      }
    }
    // If the device is new, store it
    if (isServiceNew) {
      if (knownServicesCount < MAX_DEVICES) {
        knownServices[knownServicesCount].serviceUUID = uuid;
        knownServices[knownServicesCount].count = 1;
        knownServicesCount++;
      }
    }
  }
  delay(5000);
}