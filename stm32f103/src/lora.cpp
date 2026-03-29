#include <SPI.h>
#include <LoRa.h>
#include "config.h"
#include "mylib.h"


void LoRaP2P::begin(long frequency) {
  Serial1.begin(115200);

  Serial1.println("LoRa P2P Start");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);

  if (!LoRa.begin(frequency)) {
    Serial1.println("LoRa init failed!");
    while (1);
  }

  Serial1.println("LoRa init success");
}

// ===== Send =====
void LoRaP2P::send(const char* msg) {
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();

  Serial1.print("Sent: ");
  Serial1.println(msg);
}

void LoRaP2P::sendBytes(uint8_t* data, size_t len) {
  LoRa.beginPacket();
  LoRa.write(data, len);
  LoRa.endPacket();

  Serial1.println("Sent bytes");
}

// ===== Receive =====
bool LoRaP2P::available() {
  return LoRa.parsePacket();
}

String LoRaP2P::receive() {
  String msg = "";

  while (LoRa.available()) {
    msg += (char)LoRa.read();
  }

  if (msg.length()) {
    Serial1.print("Received: ");
    Serial1.println(msg);

    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
  }

  return msg;
}

int LoRaP2P::receiveBytes(uint8_t* buffer, size_t len) {
  int i = 0;

  while (LoRa.available() && i < len) {
    buffer[i++] = LoRa.read();
  }

  return i;
}