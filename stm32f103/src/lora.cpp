// #include <Arduino.h>
// #include "mylib.h"
// #include "radio.h"
// #include "config.h"

// // ===== INIT =====
// void LoRaP2P::begin(float frequency) {
//   Serial1.begin(115200);
//   Serial1.println("LoRa P2P Start");

//   pinMode(LED_PIN, OUTPUT);
//   digitalWrite(LED_PIN, LOW);

//   int state = radio.begin(frequency);

//   if (state != RADIOLIB_ERR_NONE) {
//     Serial1.print("LoRa init failed: ");
//     Serial1.println(state);
//     while (1);
//   }

//   // 🔥 config สำคัญ (ต้องตรงกันทุก node)
//   radio.setSpreadingFactor(7);
//   radio.setBandwidth(125.0);
//   radio.setCodingRate(5);
//   radio.setOutputPower(17);

//   Serial1.println("LoRa ready");
// }

// // ===== SEND STRING =====
// void LoRaP2P::send(const char* msg) {
//   int state = radio.transmit(msg);

//   if (state == RADIOLIB_ERR_NONE) {
//     Serial1.print("Sent: ");
//     Serial1.println(msg);
//   } else {
//     Serial1.print("Send failed: ");
//     Serial1.println(state);
//   }
// }

// // ===== SEND BYTES =====
// void LoRaP2P::sendBytes(uint8_t* data, size_t len) {
//   int state = radio.transmit(data, len);

//   if (state == RADIOLIB_ERR_NONE) {
//     Serial1.println("Sent bytes");
//   } else {
//     Serial1.print("Send bytes failed: ");
//     Serial1.println(state);
//   }
// }

// // ===== RECEIVE STRING =====
// String LoRaP2P::receive() {
//   String msg = "";

//   int state = radio.receive(msg, 1000);  // timeout 1 วิ

//   if (state == RADIOLIB_ERR_NONE) {
//     Serial1.print("Received: ");
//     Serial1.println(msg);

//     digitalWrite(LED_PIN, HIGH);
//     delay(100);
//     digitalWrite(LED_PIN, LOW);
//   } 
//   else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
//     Serial1.print("Receive error: ");
//     Serial1.println(state);
//   }

//   return msg;
// }

// // ===== RECEIVE BYTES =====
// int LoRaP2P::receiveBytes(uint8_t* buffer, size_t len) {
//   int state = radio.receive(buffer, len, 1000);

//   if (state < 0 && state != RADIOLIB_ERR_RX_TIMEOUT) {
//     Serial1.print("Receive bytes error: ");
//     Serial1.println(state);
//     return 0;
//   }

//   return state;
// }