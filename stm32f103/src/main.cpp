// #define ARDUINOJSON_ENABLE_PROGMEM 1
// #define ARDUINOJSON_USE_LONG_LONG 0
// #include <Arduino.h>
// #include <SPI.h>
// #include <SD.h>
// #include <ArduinoJson.h>

// // Pin Configuration SPI2
// #define SD_MOSI PB15
// #define SD_MISO PB14
// #define SD_SCK  PB13
// #define SD_CS   PA8 // เปลี่ยนเป็น PB12 หากย้ายขาแล้ว
// #define DEBUG_SERIAL Serial1 

// void readAndDumpJson() {
//     // ใช้ชื่อไฟล์ CONFIG~1.JSO ตามที่เครื่องสแกนเจอจาก FAT32 8.3
//     File configFile = SD.open("/CONFIG~1.JSO");
    
//     if (!configFile) {
//         DEBUG_SERIAL.println("❌ Error: Could not open CONFIG~1.JSO");
//         return;
//     }

//     DEBUG_SERIAL.println("📖 File opened. Parsing content...");

//     // ใช้ JsonDocument (ArduinoJson V7)
//     JsonDocument doc;
//     DeserializationError error = deserializeJson(doc, configFile);

//     if (error) {
//         DEBUG_SERIAL.print("❌ JSON Parse failed: ");
//         DEBUG_SERIAL.println(error.f_str());
//         configFile.close();
//         return;
//     }

//     DEBUG_SERIAL.println("\n--- [ JSON DATA START ] ---");

//     // ตรวจสอบว่าเป็น Object หรือไม่
//     if (doc.is<JsonObject>()) {
//         JsonObject root = doc.as<JsonObject>();
        
//         // วนลูปอ่านทุก Key-Value Pair
//         for (JsonPair p : root) {
//             DEBUG_SERIAL.print("Key: [");
//             DEBUG_SERIAL.print(p.key().c_str());
//             DEBUG_SERIAL.print("]  =>  Value: ");
            
//             // แปลงค่าเป็น String เพื่อการแสดงผลที่ง่าย
//             DEBUG_SERIAL.println(p.value().as<String>());
//         }
//     } else {
//         DEBUG_SERIAL.println("⚠️ JSON is not an object (Check { } brackets)");
//     }

//     DEBUG_SERIAL.println("--- [  JSON DATA END  ] ---\n");

//     configFile.close();
// }

// void setup() {
//     DEBUG_SERIAL.begin(115200);
//     delay(2000);

//     DEBUG_SERIAL.println("\n==============================");
//     DEBUG_SERIAL.println("STM32 JSON DUMPER (Serial1)");
//     DEBUG_SERIAL.println("==============================");

//     // บังคับใช้ SPI2
//     SPI.setMOSI(SD_MOSI);
//     SPI.setMISO(SD_MISO);
//     SPI.setSCLK(SD_SCK);

//     if (!SD.begin(SD_CS)) {
//         DEBUG_SERIAL.println("❌ SD Card mount failed!");
//         return;
//     }
//     DEBUG_SERIAL.println("✅ SD Card Ready.");

//     // เริ่มการอ่านและ Dump ข้อมูล
//     readAndDumpJson();
// }

// void loop() {
//     // วนอ่านใหม่ทุก 10 วิน
//     }
#include <Arduino.h>
#include "mylib.h"

#define SD_MOSI PB15
#define SD_MISO PB14
#define SD_SCK  PB13
#define SD_CS   PA8
#define DEBUG_SERIAL Serial1

SDResourceManager sd(SD_MOSI, SD_MISO, SD_SCK, SD_CS);

void setup() {
    DEBUG_SERIAL.begin(115200);
    delay(2000);

    if (sd.begin()) {
if (sd.loadConfig("/CONFIG~1.JSO")) {
            DEBUG_SERIAL.print("LoRa Key: "); 
            DEBUG_SERIAL.println(sd.getLoRaKey());
        } else {
            DEBUG_SERIAL.println("❌ Load Config Failed");
        }

        // อ่านไฟล์ Log ที่เพิ่งสร้าง
        DEBUG_SERIAL.println("--- Reading Log Content ---");
        DEBUG_SERIAL.println(sd.readFile("/SYSTEM.LOG"));
    }
}

void loop() {
    // ตัวอย่างการเก็บ Log ทุกๆ 1 นาที
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 60000) {
        sd.writeLog("Heartbeat: System is running...");
        lastLog = millis();
    }
}