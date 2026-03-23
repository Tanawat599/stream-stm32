#include <Arduino.h>
#include "mylib.h"
#include "config.h"

void setup() {
  // put your setup code here, to run once:
  Serial1.begin(115200);
  Serial1.println("Hello, World!");
  pinMode(LED_PIN, OUTPUT);
  // run test from test.cpp
  test();
}

void loop() {
  Serial1.println("Hello, World!");
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  delay(1000);
}

