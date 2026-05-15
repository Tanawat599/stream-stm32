// #include <Wire.h>
// #include "mylib.h"
// #include "config.h"
// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 64

// TwoWire I2C1(PB7, PB6); 
// void Display::begin(uint8_t address) {
//   I2C1.begin();  // init I2C
//   _display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C1, -1);

//   if (!_display->begin(SSD1306_SWITCHCAPVCC, address)) {
//     Serial1.println("SSD1306 init failed");
//     while (1);
//   }

//   _display->clearDisplay();
//   _display->setTextSize(1);
//   _display->setTextColor(SSD1306_WHITE);
//   Serial1.println("SSD1306 Initialized");
// }
// // ===== Basic =====
// void Display::clear() {
//   _display->clearDisplay();
// }

// void Display::setCursor(uint8_t x, uint8_t y) {
//   _display->setCursor(x, y);
// }

// // ===== Print =====
// void Display::print(const char* msg) {
//   _display->print(msg);
// }

// void Display::printAt(uint8_t x, uint8_t y, const char* msg) {
//   _display->setCursor(x, y);
//   _display->print(msg);
// }

// // ===== Update (สำคัญมาก) =====
// void Display::update() {
//   _display->display();  // push buffer ไปจอ
// }
#include "mylib.h"
#include "config.h"
void Display::begin(uint8_t address) {
    Wire.begin();  // ใช้ I2C ของ STM32F1 (PB6=SCL, PB7=SDA)
    _display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

    if (!_display->begin(SSD1306_SWITCHCAPVCC, address)) {
        Serial.println("SSD1306 init failed");
        while (1);
    }

    _display->clearDisplay();
    _display->setTextSize(1);
    _display->setTextColor(SSD1306_WHITE);
    Serial.println("SSD1306 Initialized");
}

void Display::clear() {
    _display->clearDisplay();
}

void Display::setCursor(uint8_t x, uint8_t y) {
    _display->setCursor(x, y);
}

void Display::print(const char* msg) {
    _display->print(msg);
}

void Display::printAt(uint8_t x, uint8_t y, const char* msg) {
    _display->setCursor(x, y);
    _display->print(msg);
}

void Display::update() {
    _display->display();
}