#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "Screen.h"

#include "Vars.h"
#include "Buttons.h"
#include "NeoPixel.h"

const char* channelName = "advertised_devices";
int scanTime = 10;
BLEScan* pBLEScan;

class AdvertisedDevices : public BLEAdvertisedDeviceCallbacks {
  int numDevices;
  int minSignalStrength;

public:
  AdvertisedDevices(int minSignalStrength)
    : numDevices(0), minSignalStrength(minSignalStrength) {}

  static std::vector<std::string> seenMACAddresses;
  static std::vector<std::string> safeMACAddresses;
  static unsigned long lastClearTime;  // Declare as static


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
        Screen::displayBluetoothSpam();
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
      Serial.print(" - added to safe");
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


  static void clearSeenMACAddresses() {  // Declare as static
    seenMACAddresses.clear();
    lastClearTime = millis();
  }

  static void checkAndClearListPeriodically() {  // Declare as static
    // Clear the list every minute (adjust the interval as needed)
    if (millis() - lastClearTime > 60000) {
      Screen::displayBluetoothOn();
       NeoPixel::setNeoPixelColour("off");
      clearSeenMACAddresses();
    }
  }
};

std::vector<std::string> AdvertisedDevices::seenMACAddresses;
std::vector<std::string> AdvertisedDevices::safeMACAddresses;
unsigned long AdvertisedDevices::lastClearTime = 0;

void setup() {
  Serial.setRxBufferSize(1024);
  Serial.begin(115200);
  Serial.println("BLE Attack Detector");

  Buttons::setupButtons();

  Screen::initDisplay();
  Screen::displaySplash(3);
  Screen::displayBluetoothOff();

  NeoPixel::setupNeoPixel();

  std::vector<String> colors = { "blue", "off", "blue" };
  NeoPixel::flash(3, colors, "off");

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
