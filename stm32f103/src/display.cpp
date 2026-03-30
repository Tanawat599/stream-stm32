// #include <Wire.h>
// #include "mylib.h"
// #include "config.h"
// #define SCREEN_WIDTH 64
// #define SCREEN_HEIGHT 32

// void Display::begin(uint8_t address) {
//   Wire.setSCL(PB6);
//   Wire.setSDA(PB7);
//   Wire.begin();

//   _display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

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