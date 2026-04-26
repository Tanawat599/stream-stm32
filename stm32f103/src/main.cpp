#include <Arduino.h>
#include "mylib.h"


SDResourceManager sd(PA7, PA6, PA5, PA4); 

I2C i2cMaster;

void setup() {
    Serial1.begin(115200);
    delay(2000); 
    
    Serial1.println(F("\n============================="));
    Serial1.println(F("   I2C MASTER NODE START   "));
    Serial1.println(F("============================="));

    if (sd.begin()) {
        Serial1.println(F("SD Card Mounted Successfully!"));
        
        i2cMaster.loadConfig(sd, "/CONFI~8.JSO"); 
    } else {
        Serial1.println(F("CRITICAL ERROR: SD Card Mount Failed!"));
        Serial1.println(F("Please check wiring, 5V power, and formatted FAT32."));
    }

    i2cMaster.master_begin();
}

void loop() {
    i2cMaster.master_loop();
    
    delay(10); // คืนเวลาให้ CPU ไปทำอย่างอื่นบ้าง
}

// #include <Arduino.h>
// #include "mylib.h"

// I2C i2cSlave;

// const uint8_t SLAVE_ADDRESS = 0x40; 

// void setup() {
//     Serial1.begin(115200);
//     delay(2000); 

//     Serial1.println(F("\n============================="));
//     Serial1.println(F("   I2C SLAVE SENSOR START  "));
//     Serial1.println(F("============================="));

//     i2cSlave.slave_begin(SLAVE_ADDRESS);
    
//     Serial1.print(F("Listening for Master on Address: 0x"));
//     Serial1.println(SLAVE_ADDRESS, HEX);
// }

// void loop() {
//     i2cSlave.slave_loop();
    
//     delay(10);
// }