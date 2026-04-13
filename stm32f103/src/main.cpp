// #include <Arduino.h>
// #include "mylib.h"
// #include "config.h"

// SDResourceManager sd(SD_MOSI, SD_MISO, SD_SCK, SD_CS);
// Logger logger(&sd);

// void setup() {
//     sd.begin();

//     logger.logMsg("JOIN", "Success");

//     logger.logKV("SENSOR", 2,
//         "TEMP", 28.5, "C",
//         "HUM", 70.0, "%"
//     );
// }
// void loop() {
//     logger.logMixed("SENSOR", "Reading", 2,
//         "TEMP", 28.5, "C",
//         "HUM", 70.0, "%"
//     );

//     delay(5000);
// }

#include <Arduino.h>
#include "mylib.h"
#include "config.h"
#include <SPI.h>

// Objects
SDResourceManager sd(SD_MOSI, SD_MISO, SD_SCK, SD_CS);
Logger logger(&sd);
LoRaWan lorawan;

void setup() {
    Serial1.begin(SERIAL_BAUD);
    delay(1000); // รอให้ Serial พร้อมทำงานก่อน ไม่งั้นอาจจะมองไม่เห็นข้อความแรก

    Serial1.println("\n\n--- System Booting ---");

    // 1. สั่งปิดอุปกรณ์ทั้งคู่ก่อน
    pinMode(SD_CS, OUTPUT);  digitalWrite(SD_CS, HIGH);
    pinMode(LORA_SS_PIN, OUTPUT); digitalWrite(LORA_SS_PIN, HIGH);

    Serial1.println("--- Reading SD Card Config ---");

    // 2. ตั้งขาไปที่ SD Card (PB13-15) แล้วค่อย begin (ลบ SPI.end ทิ้งไปแล้ว)
    SPI.setSCLK(SD_SCK);
    SPI.setMISO(SD_MISO);
    SPI.setMOSI(SD_MOSI);
    SPI.begin(); 

    // อ่าน SD Card
    if (sd.begin()) {
        logger.logMsg("SYSTEM", "SD initialized");
        lorawan.loadConfig(sd, "/CONFIG~1.JSO");
        Serial1.println("SD Config Loaded.");
    } else {
        Serial1.println("SD init failed!");
    }

    Serial1.println("--- Starting LoRaWAN ---");
    
    // 3. ปิดบัสเดิม และเตรียมตัวย้ายสาย
    SPI.end(); 
    digitalWrite(SD_CS, HIGH); // สั่ง SD Card ให้เงียบ
    delay(100);

    // 4. ย้าย SPI กลับมาหา LoRa (ขา PA5-PA7)
    SPI.setSCLK(PA5);
    SPI.setMISO(PA6);
    SPI.setMOSI(PA7);
    SPI.begin(); 
    
    // เริ่มต้น LoRa
    lorawan.begin();
    lorawan.setMode(CLASS_C);

    logger.logMsg("SYSTEM", "LoRa initialized");
}
void loop() {
    // example payload send handled by LoRaWan::loop elsewhere
    lorawan.loop("Hello");
    delay(5000);
}


// #include <Arduino.h>
// #include "mylib.h"
// #include "config.h"

// LoRaWan lorawan;
// const char* payload = "Hello from loop";

// void setup() {
//   Serial1.begin(115200);
//   Serial1.println("LoraWAN Starting...");
//   lorawan.begin();
//   lorawan.setMode(CLASS_C);

// }

// void loop(){
//     lorawan.loop(payload);

// }
