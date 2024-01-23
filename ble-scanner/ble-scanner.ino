#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "Screen.h"

#include "Vars.h"
#include "Buttons.h"
#include "NeoPixel.h"

const char* channelName = "advertised_devices";
int scanTime = 10;
int resetScanTime = 120000;
BLEScan* pBLEScan;

bool operator==(const BLEAdvertisedDevice& lhs, const BLEAdvertisedDevice& rhs) {
  return const_cast<BLEAdvertisedDevice&>(lhs).getAddress().equals(const_cast<BLEAdvertisedDevice&>(rhs).getAddress());
}

class AdvertisedDevices : public BLEAdvertisedDeviceCallbacks {
  int numDevices;
  int minSignalStrength;

public:
  AdvertisedDevices(int minSignalStrength)
    : numDevices(0), minSignalStrength(minSignalStrength) {}

  static std::vector<std::string> seenMACAddresses;
  static unsigned long uniqueDeviceCount;
  static std::vector<std::string> safeMACAddresses;
  static unsigned long safeDeviceCount;
  static std::vector<BLEAdvertisedDevice> spamDevices;
  static std::vector<BLEAdvertisedDevice> flipperDevices;
  static unsigned long uniqueLockCount;
  static std::vector<std::string> uniqueFlipperDevices;
  static unsigned long lastClearTime;
  static std::string averageRSSIString;


  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("\n");
    Serial.print(advertisedDevice.getAddress().toString().c_str());

    int rssi = advertisedDevice.getRSSI();
    Serial.print("- rssi ");
    Serial.println(rssi);

    Serial.print(advertisedDevice.getName().c_str());

    if (isFlipperZero(advertisedDevice) || isEvilAppleJuice(advertisedDevice)) {
      handleLockedOn(advertisedDevice, rssi);
    } else {
      handleNonFlipperZero(advertisedDevice, rssi);
    }

    handleDisplayUpdate();
  }

  void handleLockedOn(BLEAdvertisedDevice& device, int rssi) {
    std::string rssiFlipper = std::to_string(rssi);
    Screen::displayBluetoothSpamSignal();
    Screen::updateSignalStrength(rssiFlipper.c_str());
    NeoPixel::setNeoPixelColour("red");
    flipperDevices.push_back(device);
    uniqueLockCount++;
    isUniqueFlipperDevice(device);
  }

  void handleNonFlipperZero(BLEAdvertisedDevice& device, int rssi) {
    if (flipperDevices.size() < 2) {
      if (isUniqueDevice(device)) {
        if (seenMACAddresses.size() < maxDifferentDevices) {
          handleSafeDevice(rssi);
        } else {
          handleSpamDevice(device, rssi);
        }
      }
    }
  }

  void handleSafeDevice(int rssi) {
    NeoPixel::setNeoPixelColour("green");
    if (rssi < minSignalStrength) {
      NeoPixel::setNeoPixelColour("yellow");
    }
    Screen::displayBluetoothOn();
  }

  void handleSpamDevice(BLEAdvertisedDevice& device, int rssi) {
    if (spamDevices.size() > 2) {
      Screen::displayBluetoothSpamSignal();
      Screen::updateSignalStrength(averageRSSIString.c_str());
    } else {
      Screen::displayBluetoothSpam();
    }
    spamDevices.push_back(device);
    NeoPixel::setNeoPixelColour("red");
  }

  void handleDisplayUpdate() {
    Screen::updateSectionText(generateUpdateText().c_str());
  }

  bool isFlipperZero(BLEAdvertisedDevice& device) {
    return device.haveServiceUUID() && (device.getServiceUUID().equals(BLEUUID("00003082-0000-1000-8000-00805f9b34fb")) 
    || device.getServiceUUID().equals(BLEUUID("00003081-0000-1000-8000-00805f9b34fb")) 
    || device.getServiceUUID().equals(BLEUUID("00003083-0000-1000-8000-00805f9b34fb")));
  }

  bool isEvilAppleJuice(BLEAdvertisedDevice& device) {
    return device.haveName() && device.getName() == "Airpods 69";
  }

  bool isUniqueDevice(BLEAdvertisedDevice& device) {
    // Get the MAC address as a string
    std::string macAddress = device.getAddress().toString();

    // Check if the MAC address is in the list of seen addresses
    auto itSeen = std::find(seenMACAddresses.begin(), seenMACAddresses.end(), macAddress);
    auto itSafe = std::find(safeMACAddresses.begin(), safeMACAddresses.end(), macAddress);

    if (itSeen != seenMACAddresses.end()) {
      // MAC address is already seen, move it to safeMACAddresses and remove from seenMACAddresses
      safeMACAddresses.push_back(macAddress);
      safeDeviceCount++;
      seenMACAddresses.erase(itSeen);
      uniqueDeviceCount--;
      return false;
    } else {
      if (itSafe == safeMACAddresses.end()) {
        // MAC address is unique and not in safeMACAddresses, add it to the seen list
        seenMACAddresses.push_back(macAddress);
        uniqueDeviceCount++;
        return true;
      }
      return false;
    }
  }

  bool isUniqueFlipperDevice(BLEAdvertisedDevice& device) {

    std::string deviceID = device.getServiceUUID().toString();

    auto itSeen = std::find(uniqueFlipperDevices.begin(), uniqueFlipperDevices.end(), deviceID);
    if (itSeen == uniqueFlipperDevices.end()) {
      uniqueFlipperDevices.push_back(deviceID);
      return true;
    } 
      return false;
    }


  static void spamSignal() {
    if (spamDevices.size() > 1) {
      int totalRSSI = 0;
      int count = 0;

      std::vector<BLEAdvertisedDevice> devicesToRemove;

      for (auto it = spamDevices.begin(); it != spamDevices.end(); ++it) {
        BLEAdvertisedDevice currentDevice = *it;
        int currentRSSI = currentDevice.getRSSI();

        // Check if the device has a non-similar RSSI
        if (std::abs(currentDevice.getRSSI() - currentRSSI) > 5) {  // Adjust the threshold as needed
          devicesToRemove.push_back(currentDevice);
        } else {
          totalRSSI += currentRSSI;
          count++;
        }
      }

      // Remove non-similar devices
      for (const auto& device : devicesToRemove) {
        auto it = std::find(spamDevices.begin(), spamDevices.end(), device);
        if (it != spamDevices.end()) {
          spamDevices.erase(it);
        }
      }

      if (count > 0) {
        int averageRSSI = totalRSSI / count;
        averageRSSIString = std::to_string(averageRSSI);
        Serial.print("\n");
        Serial.print("Average RSSI: ");
        Serial.println(averageRSSI);
        spamDevices.clear();
      }
    }
  }

  static void clearSeenMACAddresses() {  // Declare as static
    seenMACAddresses.clear();
    uniqueDeviceCount = 0;
    uniqueLockCount = 0;
    spamDevices.clear();
    flipperDevices.clear();
    lastClearTime = millis();
  }

  static void checkAndClearListPeriodically() {  // Declare as static
    // Clear the list every minute (adjust the interval as needed)
    if (millis() - lastClearTime > resetScanTime) {
      NeoPixel::setNeoPixelColour("off");
      clearSeenMACAddresses();
    }
  }
};

std::vector<std::string> AdvertisedDevices::seenMACAddresses;
std::vector<std::string> AdvertisedDevices::safeMACAddresses;
std::vector<BLEAdvertisedDevice> AdvertisedDevices::spamDevices;
std::vector<BLEAdvertisedDevice> AdvertisedDevices::flipperDevices;
std::vector<std::string> AdvertisedDevices::uniqueFlipperDevices;
unsigned long AdvertisedDevices::lastClearTime = 0;
unsigned long AdvertisedDevices::uniqueDeviceCount = 0;
unsigned long AdvertisedDevices::uniqueLockCount = 0;
unsigned long AdvertisedDevices::safeDeviceCount = 0;
std::string AdvertisedDevices::averageRSSIString = "";

static std::string generateUpdateText() {
        std::string updateText = AdvertisedDevices::flipperDevices.size() < 2 ? "U: " : "L: ";
        updateText += std::to_string(AdvertisedDevices::flipperDevices.size() < 2 ? AdvertisedDevices::uniqueDeviceCount : AdvertisedDevices::uniqueLockCount );
        updateText += " | S: ";
        updateText += std::to_string(AdvertisedDevices::safeDeviceCount);
        updateText += " | M: " + std::to_string(maxDifferentDevices);

        return updateText;
    }

void setup() {
  // Serial.setRxBufferSize(1024);
  Serial.begin(115200);
  Serial.println("BLE Attack Detector");

  Buttons::setupButtons();

  Screen::initDisplay();
  Screen::displaySplash(3);
  Screen::displayBluetoothOff();

  NeoPixel::setupNeoPixel();

  std::vector<String> colors = { "blue", "off", "blue" };
  NeoPixel::flash(2, colors, "off");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDevices(-70));  // Sensitivty settings - Adjust values accordingly
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void startScan(BLEScanResults scanResults) {
  pBLEScan->start(0, startScan, false);
}

enum State {
  IDLE,
  SCANNING,
  ADJUST_MAX_DEVICES,
  SCAN_STARTED,
  SCAN_STOPPED
};

// Initialize the state machine with an initial state
State currentState = IDLE;
State currentScanState = SCAN_STOPPED;

void loop() {
  Buttons::updateButtons();

  switch (currentState) {
    case IDLE:
      if (lastPressedButton == 0 && currentScanState != SCAN_STARTED) {
        Serial.print("\n");
        pBLEScan->start(10, startScan, false);
        currentScanState = SCAN_STARTED;
        currentState = SCANNING;
      }
      if (lastPressedButton == 2 || lastPressedButton == 3  && currentScanState != SCAN_STARTED) {
        Screen::updateSectionText(generateUpdateText().c_str());
      }
      break;

    case SCANNING:
      // Continue scanning process
      if (AdvertisedDevices::flipperDevices.size() < 1) {
        AdvertisedDevices::spamSignal();
      }
      AdvertisedDevices::checkAndClearListPeriodically();

      // Check for button press to stop the scan
      if (lastPressedButton == 1 && currentScanState != SCAN_STOPPED) {
        AdvertisedDevices::clearSeenMACAddresses();
        pBLEScan->stop();
        pBLEScan->clearResults();
        Screen::updateSectionText(generateUpdateText().c_str());
        currentScanState = SCAN_STOPPED;
        currentState = IDLE;
      }
      break;
  }
}

