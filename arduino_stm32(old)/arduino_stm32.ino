#include "mylib.h"
#include "config.h"

Display display;
TwoWire I2C1(PB7, PB6);
void setup() {
  Serial1.begin(115200);
  display.begin(0x3C); // OLED I2C address 0x3C
  display.clear();
  display.printAt(0,0,"Hello STM32");
  display.update();
}

void loop() {
  display.clear();
  display.printAt(0,0,"Running...");
  display.update();
  Serial1.println("hi");
  delay(1000);
}