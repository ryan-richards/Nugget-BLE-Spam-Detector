#include "Arduino.h"
#include "Screen.h"
#include "SH1106.h"
#include "graphics.h"

SH1106Wire display(0x3C, 35, 36);

const char* Screen::sectionText = "NUGGET BLE DETECTOR";
const char* Screen::signalRSSI = "-00";

Screen::Screen() {
}

void Screen::initDisplay() {
  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(DejaVu_Sans_Mono_10);
}

void Screen::displaySplash(uint8_t sec) {
  display.clear();
  display.drawString(35, 25, "BLE DETECT");
  display.drawString(52, 40, "@catzpacho");
  display.drawString(20, 40, "v1.1");
  display.display();
  delay(sec * 700);
}

void Screen::displayBluetoothOff() {
  display.clear();
  display.drawXbm(0, 0, 128, 64, plain_bits);
  display.drawLine(0, 53, 127, 53);
  display.drawLine(0, 54, 127, 54);
  display.drawString(0, 54, sectionText);
  display.display();
}

void Screen::displayBluetoothOn() {
  display.clear();
  display.drawXbm(0, 0, 128, 64, blue_bits);
  display.drawLine(0, 53, 127, 53);
  display.drawLine(0, 54, 127, 54);
  display.drawString(0, 54, sectionText);
  display.display();
}

void Screen::displayBluetoothSpam() {
  display.clear();
  display.drawXbm(0, 0, 128, 64, spam_bits);
  display.drawLine(0, 53, 127, 53);
  display.drawLine(0, 54, 127, 54);
  display.drawString(0, 54, sectionText);
  display.display();
}

void Screen::displayBluetoothSpamSignal() {
  display.clear();
  display.drawString(55, 16, signalRSSI);
  display.drawXbm(0, 0, 128, 64, spam_bits_signal);
  display.drawLine(0, 53, 127, 53);
  display.drawLine(0, 54, 127, 54);
  display.drawString(0, 54, sectionText);
  display.display();
}

void Screen::updateSectionText(const char* newText) {
  sectionText = newText;
  display.setColor(BLACK);
  display.fillRect(0, 55, 128, 9);
  display.setColor(WHITE);

  // Draw the new text in the same location
  display.drawString(0, 54, newText);
  display.display();
}

void Screen::updateSignalStrength(const char* newText) {
  signalRSSI = newText;
  const int rectWidth = 10;
  const int rectHeight = 10;
  
  const int rectX = 57;  // Update x-coordinate
  const int rectY = 70;  // Update y-coordinate

  display.setColor(BLACK);
  display.fillRect(rectX, rectY, rectWidth, rectHeight);
  display.setColor(WHITE);

  // Draw the new text in the center of the rectangle
  int textX = rectX + (rectWidth - display.getStringWidth(newText)) / 2 - 18;
  int textY = rectY + rectHeight / 2 - 20;  // Adjust vertically to center the text

  display.drawString(textX, textY, newText);
  display.display();
}


