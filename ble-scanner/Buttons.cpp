// Buttons.cpp

#include "Buttons.h"
#include <Bounce2.h>
#include "Vars.h"
#include "Screen.h"
#include "NeoPixel.h"

Bounce b = Bounce();

const int buttonPins[] = {up_btn, dn_btn, lt_btn, rt_btn};
const int numButtons = sizeof(buttonPins) / sizeof(buttonPins[0]);

Bounce buttons[numButtons];

int lastPressedButton = -1;
int maxDifferentDevices = 17;

int lastClearTime = 0;
int lastPressedButton2 = 1000;

Buttons::Buttons() {
}

void Buttons::setupButtons() {
  for (int i = 0; i < numButtons; i++) {
    buttons[i].attach(buttonPins[i], INPUT_PULLUP);
    buttons[i].interval(5);
  }
}

void Buttons::updateButtons() {
  for (int i = 0; i < numButtons; i++) {
    buttons[i].update();

    if (buttons[i].fell()) {
      lastPressedButton = i;

      switch (i) {
        case 0: // Button 0 (up_btn) was pressed
            NeoPixel::setNeoPixelColour("blue");
            Screen::displayBluetoothOn();
          break;
        case 1: // Button 1 (dn_btn) was pressed
          NeoPixel::setNeoPixelColour("off");
          Screen::displayBluetoothOff();
          break;
        case 2: // Button 2 (lt_btn) was pressed
          maxDifferentDevices = (maxDifferentDevices - 1 + 99) % 99;
          break;
        case 3: // Button 3 (rt_btn) was pressed
          maxDifferentDevices = (maxDifferentDevices + 1) % 99;
          break;
      }
    }
  }
}
