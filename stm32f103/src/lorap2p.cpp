#include "lorap2p.h"

LoRaP2P::LoRaP2P(): module(LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_DIO1_PIN), radio(&module) {}

int16_t LoRaP2P::begin(float frequency) {
  // Debug serial
  Serial1.begin(SERIAL_BAUD);
  Serial1.println("LoRa P2P Start");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  int16_t state = radio.begin(frequency);
  if (state != RADIOLIB_ERR_NONE) {
    Serial1.print("LoRa init failed: ");
    Serial1.println(state);
    return state;
  }

  radio.setSpreadingFactor(7);
  radio.setBandwidth(125.0);
  radio.setCodingRate(5);
  radio.setOutputPower(17);

  Serial1.println("LoRa ready");
  return RADIOLIB_ERR_NONE;
}

int16_t LoRaP2P::send(const char* msg) {
  int16_t state = radio.transmit(msg);
  if (state == RADIOLIB_ERR_NONE) {
    Serial1.print("Sent: ");
    Serial1.println(msg);
  } else {
    Serial1.print("Send failed: ");
    Serial1.println(state);
  }
  return state;
}

int16_t LoRaP2P::sendBytes(const uint8_t* data, size_t len) {
  int16_t state = radio.transmit(data, len);
  if (state == RADIOLIB_ERR_NONE) {
    Serial1.println("Sent bytes");
  } else {
    Serial1.print("Send bytes failed: ");
    Serial1.println(state);
  }
  return state;
}

String LoRaP2P::receive(uint32_t timeout) {
  String msg = "";
  int16_t state = radio.receive(msg, timeout);

  if (state == RADIOLIB_ERR_NONE) {
    Serial1.print("Received: ");
    Serial1.println(msg);

    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
  } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
    Serial1.print("Receive error: ");
    Serial1.println(state);
  }

  return msg;
}

int16_t LoRaP2P::receiveBytes(uint8_t* buffer, size_t len, uint32_t timeout) {
  int16_t state = radio.receive(buffer, len, timeout);
  if (state < 0 && state != RADIOLIB_ERR_RX_TIMEOUT) {
    Serial1.print("Receive bytes error: ");
    Serial1.println(state);
    return state;
  }
  return state;
}
