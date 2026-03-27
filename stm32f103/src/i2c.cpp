#include <Wire.h>
#include <Arduino.h>
#include "mylib.h"
#include "config.h"

#define SLAVE_ADDR 0x08

char buffer[20];
volatile int idx = 0;
volatile bool newData = false;  

// interrupt
void receiveEvent(int howMany) {
  idx = 0;

  while (Wire.available() && idx < sizeof(buffer) - 1) {
    buffer[idx++] = Wire.read();
  }
  buffer[idx] = '\0';

  newData = true;  

void I2C::slave_begin() {
  Wire.begin(SLAVE_ADDR);
  Wire.onReceive(receiveEvent);
  Serial1.begin(115200); 
}

void I2C::slave_loop() {
  if (newData) {
    noInterrupts();  // 🔒 กัน interrupt แทรกตอน copy

    char temp[20];
    strcpy(temp, buffer);  // copy ออกมาใช้

    newData = false;

    interrupts();  // 🔓 เปิด interrupt กลับ

    Serial1.print("Received: ");
    Serial1.println(temp);
  }
}