// #include <Arduino.h>
// #include "mylib.h"

// // กำหนดขา SPI สำหรับ SD Card (ปรับให้ตรงกับบอร์ดของคุณ)
// // มาตรฐาน SPI1 ของ STM32F103: MOSI=PA7, MISO=PA6, SCK=PA5, CS=PA4
// SDResourceManager sd(PA7, PA6, PA5, PA4); 

// I2C i2cMaster;

// void setup() {
//     // เริ่มต้น Serial1 (PA9=TX, PA10=RX) สำหรับดู Log
//     Serial1.begin(115200);
//     delay(2000); 
    
//     Serial1.println(F("\n============================="));
//     Serial1.println(F("   I2C MASTER NODE START   "));
//     Serial1.println(F("============================="));

//     // 1. เริ่มต้น SD Card (ต้องเสียบ SD Card แบบ FAT32 ให้แน่น)
//     if (sd.begin()) {
//         Serial1.println(F("SD Card Mounted Successfully!"));
        
//         // 2. โหลด Config จากไฟล์ (ใช้เทคนิค Malloc กัน RAM เต็ม)
//         i2cMaster.loadConfig(sd, "/CONFI~8.JSO"); 
//     } else {
//         Serial1.println(F("CRITICAL ERROR: SD Card Mount Failed!"));
//         Serial1.println(F("Please check wiring, 5V power, and formatted FAT32."));
//     }

//     // 3. เปิดระบบ I2C Master (เซ็ต Clock และเปิด Hardware)
//     i2cMaster.master_begin();
// }

// void loop() {
//     // 4. วนลูปอ่านค่า (มีระบบหน่วงเวลาและ Auto-Reset I2C เมื่อค้าง)
//     i2cMaster.master_loop();
    
//     delay(10); // คืนเวลาให้ CPU ไปทำอย่างอื่นบ้าง
// }

#include <Arduino.h>
#include "mylib.h"

I2C i2cSlave;

// กำหนด Address ของ Slave ให้ตรงกับใน JSON Config ของ Master
const uint8_t SLAVE_ADDRESS = 0x40; 

void setup() {
    Serial1.begin(115200);
    delay(2000); 

    Serial1.println(F("\n============================="));
    Serial1.println(F("   I2C SLAVE SENSOR START  "));
    Serial1.println(F("============================="));

    // 1. เริ่มต้น I2C โหมด Slave พร้อมผูกฟังก์ชัน onReceive และ onRequest
    i2cSlave.slave_begin(SLAVE_ADDRESS);
    
    Serial1.print(F("Listening for Master on Address: 0x"));
    Serial1.println(SLAVE_ADDRESS, HEX);
}

void loop() {
    // 2. คอยเช็คและปริ้นข้อความว่า Master ส่งคำสั่งมาขอ Register ตัวไหน
    i2cSlave.slave_loop();
    
    delay(10);
}