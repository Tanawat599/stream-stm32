#include <Arduino.h>
#include "mylib.h"
#include "config.h"

I2C i2c;

void setup() {
  i2c.slave_begin(0x08);
}

void loop() {
  i2c.slave_loop();
}