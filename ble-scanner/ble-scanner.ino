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
  static std::vector<std::string> safeMACAddresses;
  static std::vector<BLEAdvertisedDevice> spamDevices;
  static unsigned long lastClearTime;
  static std::string averageRSSIString;


  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("\n");
    Serial.print(advertisedDevice.getAddress().toString().c_str());

    // Check proximity based on signal strength
    int rssi = advertisedDevice.getRSSI();
    Serial.print("- rssi ");
    Serial.println(rssi);

    // Check for different MAC addresses
    if (isUniqueDevice(advertisedDevice)) {
      Serial.print(" - Unique MAC Address detected!");
      if (seenMACAddresses.size() < maxDifferentDevices) {
        NeoPixel::setNeoPixelColour("green");
        if (rssi < minSignalStrength) {
          NeoPixel::setNeoPixelColour("yellow");
        }
        Screen::displayBluetoothOn();
      } else {
        if (spamDevices.size() > 5) {
          Screen::displayBluetoothSpamSignal();
          Screen::updateSignalStrength(averageRSSIString.c_str());
        } else {
          Screen::displayBluetoothSpam();
        }
        spamDevices.push_back(advertisedDevice);
        NeoPixel::setNeoPixelColour("red");
      }
    }

    Screen::updateSectionText(("U: " + std::to_string(seenMACAddresses.size()) + " | S: " + std::to_string(safeMACAddresses.size()) + " | M: " + std::to_string(maxDifferentDevices)).c_str());
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
      seenMACAddresses.erase(itSeen);
      return false;
    } else {
      if (itSafe == safeMACAddresses.end()) {
        // MAC address is unique and not in safeMACAddresses, add it to the seen list
        seenMACAddresses.push_back(macAddress);
        return true;
      }
      return false;
    }
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
            if (std::abs(currentDevice.getRSSI() - currentRSSI) > 5) { // Adjust the threshold as needed
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
        }
    }
}

  static void clearSeenMACAddresses() {  // Declare as static
    seenMACAddresses.clear();
    spamDevices.clear();
    lastClearTime = millis();
  }

  static void checkAndClearListPeriodically() {  // Declare as static
    // Clear the list every minute (adjust the interval as needed)
    if (millis() - lastClearTime > resetScanTime) {
      Screen::displayBluetoothOn();
       NeoPixel::setNeoPixelColour("off");
      clearSeenMACAddresses();
    }
  }
};

std::vector<std::string> AdvertisedDevices::seenMACAddresses;
std::vector<std::string> AdvertisedDevices::safeMACAddresses;
std::vector<BLEAdvertisedDevice> AdvertisedDevices::spamDevices;
unsigned long AdvertisedDevices::lastClearTime = 0;
std::string AdvertisedDevices::averageRSSIString = "";

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

void loop() {
  Buttons::updateButtons();
  
  if (lastPressedButton == 0) { 
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    pBLEScan->clearResults();
    delay(1000);
    AdvertisedDevices::spamSignal();
    AdvertisedDevices::checkAndClearListPeriodically();
  }

  if (lastPressedButton == 2 || lastPressedButton == 3 ) {
    Screen::updateSectionText(("U: " + std::to_string(AdvertisedDevices::seenMACAddresses.size()) + " | S: " + std::to_string(AdvertisedDevices::safeMACAddresses.size()) + " | M: " + std::to_string(maxDifferentDevices)).c_str());
  }

  if (lastPressedButton == 1) { 
    pBLEScan->clearResults();
    delay(1000);
    AdvertisedDevices::checkAndClearListPeriodically();
    NeoPixel::setNeoPixelColour("off");
  }


}
