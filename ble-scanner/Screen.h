#include "Arduino.h"

#pragma once

class Screen {
public:
    Screen();
    static void initDisplay();
    static void displayBluetoothOff();
    static void displayBluetoothOn();
    static void displayBluetoothSpam();
    static void displayBluetoothSpamSignal();
    static void displaySplash(uint8_t sec);
    static void updateSectionText(const char* newText);
    static void updateSignalStrength(const char* newText);

private:
    static const char* sectionText;
    static const char* signalRSSI;
};
