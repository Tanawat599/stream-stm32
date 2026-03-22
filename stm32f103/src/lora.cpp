#include <SPI.h>
#include <LoRa.h>
#include "config.h"

void LoRaP2P::begin() {
  STM32_SERIAL.begin(115200);
  delay(100);
  STM32_SERIAL.println("LoRa Sender (MySerial)");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);

  if (!LoRa.begin(923E6)) {
    STM32_SERIAL.println("LoRa init failed!");
    while (1);
  }

  STM32_SERIAL.println("LoRa init success");
}

void LoRaP2P::send() {
  LoRa.beginPacket();
  LoRa.print("Hello, LoRa!");
  LoRa.endPacket();
  STM32_SERIAL.println("Message sent!");
  delay(2000);
}

void LoRaP2P::receive() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    STM32_SERIAL.print("Received packet: '");
    while (LoRa.available()) {
      STM32_SERIAL.print((char)LoRa.read());
    }
    STM32_SERIAL.println("'");
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
  }
}