#include <SPI.h>
#include <LoRa.h>
#include "config.h"
#include "mylib.h"

void LoRaP2P::begin() {
  Serial1.begin(115200);
  delay(100);
  Serial1.println("LoRa Sender (MySerial)");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);

  if (!LoRa.begin(923E6)) {
    Serial1.println("LoRa init failed!");
    while (1);
  }

  Serial1.println("LoRa init success");
}

void LoRaP2P::send() {
  LoRa.beginPacket();
  LoRa.print("Hello, LoRa!");
  LoRa.endPacket();
  Serial1.println("Message sent!");
  delay(2000);
}

void LoRaP2P::receive() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial1.print("Received packet: '");
    while (LoRa.available()) {
      Serial1.print((char)LoRa.read());
    }
    Serial1.println("'");
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
  }
}