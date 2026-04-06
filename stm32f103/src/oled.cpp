#include "mylib.h"
#include "config.h"

#include <U8g2lib.h>
#include <Wire.h>

// 🔹 Full buffer I2C (SSD1306 128x64)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
    U8G2_R0,               // rotation
    U8X8_PIN_NONE,         // reset pin
    PB6,                   // SCL (STM32 default)
    PB7                    // SDA (STM32 default)
);

void OLED::begin() {
    Wire.begin();
    u8g2.begin();
}

void OLED::clear() {
    u8g2.clearBuffer();
}

void OLED::update() {
    u8g2.sendBuffer();
}

// ---------------- TEXT ----------------
void OLED::setFont(const uint8_t* font) {
    u8g2.setFont(font);
}

void OLED::print(const char* text, int x, int y) {
    u8g2.drawStr(x, y, text);
}

void OLED::println(const char* text, int x, int y) {
    u8g2.drawStr(x, y, text);
}

void OLED::drawStr(int x, int y, const char* text) {
    u8g2.drawStr(x, y, text);
}

// ---------------- DRAW ----------------
void OLED::drawPixel(int x, int y) {
    u8g2.drawPixel(x, y);
}

void OLED::drawLine(int x1, int y1, int x2, int y2) {
    u8g2.drawLine(x1, y1, x2, y2);
}

void OLED::drawBox(int x, int y, int w, int h) {
    u8g2.drawBox(x, y, w, h);
}

void OLED::drawFrame(int x, int y, int w, int h) {
    u8g2.drawFrame(x, y, w, h);
}

// ---------------- SETTINGS ----------------
void OLED::setContrast(uint8_t value) {
    u8g2.setContrast(value);
}

void OLED::setFlip(bool flip) {
    u8g2.setFlipMode(flip ? 1 : 0);
}

// ---------------- TEST ----------------
void OLED::test() {
    clear();
    setFont(u8g2_font_5x7_tr);

    drawStr(0, 10, "OLED TEST");
    drawStr(0, 20, "HELLO ARDUINO");

    drawFrame(0, 0, 128, 64);
    drawLine(0, 32, 128, 32);

    update();
}