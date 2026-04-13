
// #include <Arduino.h>
// #include "mylib.h"
// #include "config.h"

// SDResourceManager sd(SD_MOSI, SD_MISO, SD_SCK, SD_CS);
// Logger logger(&sd);
// LoRaWan lorawan;

// void setup() {
//     Serial1.begin(SERIAL_BAUD);
//     delay(1000); 

//     pinMode(SD_CS, OUTPUT);       digitalWrite(SD_CS, HIGH);
//     pinMode(LORA_SS_PIN, OUTPUT); digitalWrite(LORA_SS_PIN, HIGH);

//     Serial1.println("\n--- System Booting ---");

//     // ==========================================
//     // Phase 1: SD Card
//     // ==========================================
//     if (sd.begin()) {
//         logger.logMsg("SYSTEM", "SD initialized");
//         lorawan.loadConfig(sd, "/CONFIG~1.JSO");
//         sd.end();
//     } else {
//         Serial1.println("SD init failed!");
//     }

//     // ==========================================
//     // Phase 2: LoRaWAN
//     // ==========================================
//     lorawan.begin();
//     lorawan.setMode(CLASS_C);
//     logger.logMsg("SYSTEM", "LoRa initialized");
// }

// void loop() {
//     // โค้ดลูปทำงานได้ตามปกติ
//     lorawan.loop("Hello");
//     delay(5000);
// }
#include <Arduino.h>
void setup() {
    Serial1.begin(115200);
    delay(2000);

    Serial1.println("--- STM32 Unique ID (Direct Memory) ---");

    // ตำแหน่ง Address ของ UID สำหรับตระกูล STM32F1xx
    uint32_t *uidAddress = (uint32_t *)0x1FFFF7E8;

    // อ่านข้อมูล 3 ชุด (ชุดละ 32-bit)
    uint32_t word0 = uidAddress[0];
    uint32_t word1 = uidAddress[1];
    uint32_t word2 = uidAddress[2];

    Serial1.print("Device UID: ");
    Serial1.print(word0, HEX);
    Serial1.print(word1, HEX);
    Serial1.println(word2, HEX);
}

void loop() {
}