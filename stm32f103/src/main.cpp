#include <Arduino.h>
#include "mylib.h"
#include "config.h"
I2C i2c;

void setup() {
  i2c.master_begin();
}

void loop() {
  i2c.master_send(SLAVE_ADDR_1, "Hello Slave!");
  delay(1000);

  uint8_t data[] = {0x01, 0x02, 0x03};
  i2c.master_sendBytes(SLAVE_ADDR_2, data, sizeof(data));
  delay(1000);
}
// I2C i2c;

// void setup() {
//   i2c.slave_begin(SLAVE_ADDR_1);  // ใส่ address ของ slave
// }

// void loop() {
//   i2c.slave_loop();  // ต้องเรียกทุก loop เพื่อตรวจสอบข้อมูลใหม่
// }




// #include <Arduino.h>
// #include "mylib.h"   
// #include "config.h"  

// RS485 rs485(Serial2, RS485_DE_PIN, RS485_RE_PIN);

// void setup() {

//   Serial1.begin(115200);
//   Serial1.println("RS485 Test Starting...");


//   rs485.begin(9600);
// }
// void loop() {

//   if (rs485.available()) {

//     String msg = rs485.receive();
//     Serial1.print("Received: ");
//     Serial1.println(msg);


//     uint8_t buffer[10];
//     int n = rs485.receiveBytes(buffer, 10);
//     if (n > 0) {
//       Serial1.print("Received bytes: ");
//       for (int i = 0; i < n; i++) {
//         Serial1.print(buffer[i], HEX);
//         Serial1.print(" ");
//       }
//       Serial1.println();
//     }
//   }
// }
// void loop() {

//   rs485.send("Hello RS485!");


//   uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
//   rs485.sendBytes(data, sizeof(data));

//   delay(1000); // รอ 1 วินาที
// }



// #include <Arduino.h>
// #include "mylib.h"

// OLED oled;

// void setup() {
//     oled.begin();
//     oled.clear();
//     oled.setFont(u8g2_font_5x7_tr);

//     // แสดงข้อความ
//     oled.print("Hello, World!", 0, 10);
//     oled.println("Line 2", 0, 20);

//     // อัปเดต buffer
//     oled.update();
// }

// void loop() {
//     // oled.clear();
//     // oled.drawFrame(0, 0, 128, 64);
//     // oled.drawLine(0, 32, 128, 32);
//     // oled.update();
//     // delay(1000);
// }

// #include <Arduino.h>
// #include "lorap2p.h"

// LoRaP2P lora;   // 🔥 สร้าง object

// // Map some common RadioLib error codes to human-readable text
// const char* radioErrToStr(int16_t err) {
//   switch(err) {
//     case RADIOLIB_ERR_NONE: return "No error";
//     case RADIOLIB_ERR_UNKNOWN: return "Unknown error";
//     case RADIOLIB_ERR_CHIP_NOT_FOUND: return "Chip not found";
//     case RADIOLIB_ERR_INVALID_FREQUENCY: return "Invalid frequency (check module and MHz value)";
//     case RADIOLIB_ERR_INVALID_SPREADING_FACTOR: return "Invalid spreading factor";
//     case RADIOLIB_ERR_INVALID_BANDWIDTH: return "Invalid bandwidth";
//     default: return "Other RadioLib error";
//   }
// }

// void setup() {
//   // ถ้าใช้ SX1276 บอร์ดส่วนใหญ่อยู่ในย่าน 433 / 868 / 915 MHz
//   float tryFreqs[] = {915.0, 868.0, 433.0, 923.0};
//   const size_t nFreqs = sizeof(tryFreqs) / sizeof(tryFreqs[0]);

//   Serial1.begin(115200);
//   int16_t state = RADIOLIB_ERR_UNKNOWN;
//   for (size_t i = 0; i < nFreqs; ++i) {
//     float f = tryFreqs[i];
//     Serial1.print("Trying frequency: ");
//     Serial1.println(f);
//     state = lora.begin(f);
//     if (state == RADIOLIB_ERR_NONE) {
//       Serial1.print("LoRa initialized at ");
//       Serial1.print(f);
//       Serial1.println(" MHz");
//       break;
//     } else {
//       Serial1.print("Init failed: ");
//       Serial1.print(state);
//       Serial1.print(" -> ");
//       Serial1.println(radioErrToStr(state));
//     }
//   }

//   if (state != RADIOLIB_ERR_NONE) {
//     Serial1.println("All frequency attempts failed. Check wiring, chip variant (SX1276) and SPI pins.");
//     while (1);
//   }
// }

// void loop() {

//   // ===== ส่งทุก 3 วิ =====
//   static unsigned long lastSend = 0;
//   if (millis() - lastSend > 3000) {
//     lora.send("Hello from STM32");
//     lastSend = millis();
//   }

//   // ===== รับ =====
//   String msg = lora.receive(1000);   // timeout 1 วิ

//   if (msg.length()) {
//     Serial1.println("Got message in main!");
//   }
// }