#include "mylib.h"

// Initialize static members
volatile bool I2C::_newData = false;
char I2C::_buffer[32];
volatile int I2C::_idx = 0;

// ==========================================
// ===== Master Logic =====
// ==========================================

void I2C::master_begin() {
    Wire.begin();
    if (_frequency > 0) {
        Wire.setClock(_frequency); 
    }
    Serial1.print(F("I2C Master Initialized at "));
    Serial1.print(_frequency);
    Serial1.println(F(" Hz"));
}

void I2C::loadConfig(SDResourceManager& sd, const char* path) {
    Serial1.println(F("I2C: Loading Config..."));
    Serial1.flush();

    File file = SD.open(path);
    if (!file) {
        Serial1.println(F("I2C: ERROR - Failed to open file!"));
        return;
    }

    size_t fileSize = file.size();
    Serial1.printf("I2C: File opened. Size: %d bytes\n", fileSize);
    Serial1.flush();

    if (fileSize == 0 || fileSize > 4096) {
        Serial1.println(F("I2C: ERROR - File is empty or too large!"));
        file.close();
        return;
    }

    char* buffer = (char*)malloc(fileSize + 1);
    if (!buffer) {
        Serial1.println(F("I2C: ERROR - Out of RAM!"));
        file.close();
        return;
    }

    file.read((uint8_t*)buffer, fileSize);
    buffer[fileSize] = '\0';
    file.close();

    Serial1.println(F("I2C: File read to RAM. Parsing JSON..."));
    Serial1.flush();

    JsonDocument doc; 
    DeserializationError err = deserializeJson(doc, buffer);
    
    // สำคัญ: ห้าม free(buffer) ตรงนี้เด็ดขาด ปล่อยให้มันมีชีวิตอยู่จนจบฟังก์ชัน!

    if (err) {
        Serial1.print(F("I2C: JSON Parse Failed! Error: "));
        Serial1.println(err.c_str());
        free(buffer); // คืน Memory ก่อนจบการทำงาน
        return;
    }

    Serial1.println(F("I2C: JSON Parsed Successfully. Extracting data..."));
    Serial1.flush();

    JsonObject hardware = doc["hardware"];
    if (hardware.isNull()) {
        Serial1.println(F("I2C: Cannot find 'hardware' key in JSON!"));
        free(buffer);
        return;
    }

    JsonObject i2c = hardware["i2c"]; 
    if (i2c.isNull()) {
        Serial1.println(F("I2C: Cannot find 'i2c' key inside 'hardware'!"));
        free(buffer);
        return;
    }

    _enabled = i2c["enable"] | false;
    _frequency = i2c["frequency"] | 100000;
    _interval_ms = i2c["interval_ms"] | 2000;
    _max_retry = i2c["max_retry"] | 3;
    
    // เอา Wire.setClock ออกจากตรงนี้ก่อน ไปตั้งค่าตอน master_begin แทน

    _devices.clear();
    JsonArray devices = i2c["devices"];
    for (JsonObject dev : devices) {
        I2C_Device d;
        d.name = dev["name"].as<String>();
        
        // การเช็คว่าถ้าไม่มี address ให้ข้ามไป จะได้ไม่ค้าง
        if (!dev["address"].isNull()) {
            d.address = parseHex(dev["address"].as<const char*>());
        }

        JsonArray channels = dev["channels"];
        for (JsonObject ch : channels) {
            I2C_Channel c;
            c.name = ch["name"].as<String>();
            if (!ch["register"].isNull()) {
                c.reg = parseHex(ch["register"].as<const char*>());
            }
            c.length = ch["length"];
            c.byte_order = ch["byte_order"].as<String>();
            c.scale = ch["scale"] | 1.0;
            d.channels.push_back(c);
        }
        _devices.push_back(d);
    }

    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf), "I2C Config Loaded -> EN:%d, Freq:%lu, Int:%lu, Retry:%d, Devs:%d",
             _enabled, _frequency, _interval_ms, _max_retry, _devices.size());
             
    Serial1.println(logBuf);
    
    if (_enabled && !_devices.empty()) {
        for (const auto& dev : _devices) {
            Serial1.printf(" - Device: %s [0x%02X] (%d channels)\n", dev.name.c_str(), dev.address, dev.channels.size());
            for (const auto& ch : dev.channels) {
                Serial1.printf("    > CH: %s [Reg:0x%02X, Len:%d, Order:%s, Scale:%.2f]\n", 
                               ch.name.c_str(), ch.reg, ch.length, ch.byte_order.c_str(), ch.scale);
            }
        }
    }

    // ภารกิจดึงข้อมูลเสร็จสิ้น คืน RAM กลับให้ระบบได้แล้ว!
    free(buffer); 
    Serial1.println(F("I2C: Initialization Complete!"));
}

void I2C::master_loop() {
    if (!_enabled || _devices.empty()) return;
    
    if (millis() - _lastPoll >= _interval_ms) {
        _lastPoll = millis();

        for (auto& dev : _devices) {
            Serial1.print(F("\n>>> Polling Device: "));
            Serial1.print(dev.name);
            Serial1.print(F(" [0x"));
            Serial1.print(dev.address, HEX);
            Serial1.println(F("]"));

            for (auto& ch : dev.channels) {
                uint8_t raw[4] = {0}; 
                
                // เริ่มกระบวนการอ่าน
                if (readRegister(dev.address, ch.reg, raw, ch.length)) {
                    uint32_t rawVal = processRawData(raw, ch.length, ch.byte_order);
                    float finalVal = (float)rawVal * ch.scale;
                    
                    Serial1.print(F("    - "));
                    Serial1.print(ch.name);
                    Serial1.print(F(": "));
                    Serial1.println(finalVal); // Serial.print รองรับ float ได้ปกติ
                } else {
                    Serial1.print(F("    - "));
                    Serial1.print(ch.name);
                    Serial1.println(F(": READ FAILED!"));
                }
            }
        }
    }
}

bool I2C::readRegister(uint8_t devAddr, uint8_t regAddr, uint8_t* buffer, uint8_t len) {
    for (int r = 0; r < _max_retry; r++) {
        Wire.beginTransmission(devAddr);
        Wire.write(regAddr);
        
        // ใช้ false (Repeated Start) ซึ่งเป็นมาตรฐานที่ Sensor ส่วนใหญ่รวมถึง Slave ต้องการ
        uint8_t error = Wire.endTransmission(false); 
        
        if (error == 0) { // 0 = ส่ง Address Register สำเร็จ
            // สั่งขอข้อมูลจาก Slave
            uint8_t bytesReceived = Wire.requestFrom((uint8_t)devAddr, (uint8_t)len);
            
            if (bytesReceived == len) {
                for (uint8_t i = 0; i < len; i++) {
                    buffer[i] = Wire.read();
                }
                return true; // อ่านสำเร็จ! ออกจากลูปได้เลย
            } else {
                Serial1.print(F(" [Err: Req "));
                Serial1.print(len);
                Serial1.print(F(" bytes, got "));
                Serial1.print(bytesReceived);
                Serial1.print(F("] "));
            }
        } else {
            Serial1.print(F(" [Err Code: "));
            Serial1.print(error);
            Serial1.print(F("] "));
            
            // ======================================================
            // ท่าไม้ตาย STM32: ถ้าเจอ Error 4 (Bus ค้าง) ให้รีเซ็ต I2C ใหม่ทันที
            // ======================================================
            if (error == 4) {
                Serial1.print(F("[Resetting I2C Bus...] "));
                Wire.end();                // ปิดฮาร์ดแวร์ I2C
                delay(10);
                Wire.begin();              // เปิดใหม่
                Wire.setClock(_frequency); // คืนค่า Clock
            }
        }
        
        delay(50); // หน่วงเวลาให้ฝั่ง Slave หายใจก่อนพยายามใหม่ (Retry)
    }
    return false; // ลองครบจำนวน max_retry แล้วก็ยังไม่ได้
}
uint32_t I2C::processRawData(uint8_t* data, uint8_t len, String order) {
    uint32_t val = 0;
    if (order == "AB" || order == "ABCD") { // Big Endian
        for (int i = 0; i < len; i++) val = (val << 8) | data[i];
    } else { // Little Endian (BA / DCBA)
        for (int i = len - 1; i >= 0; i--) val = (val << 8) | data[i];
    }
    return val;
}

uint8_t I2C::parseHex(const char* str) {
    if (!str) return 0;
    return (uint8_t)strtol(str, NULL, 16);
}

// ==========================================
// ===== Slave Logic (Sensor Simulator) =====
// ==========================================

static uint16_t sensorTemp = 255; 
static uint16_t sensorHumid = 600;

void I2C::slave_begin(uint8_t address) {
    Wire.begin(address);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent); // <--- ผูกฟังก์ชันตอบกลับ
    
    Serial1.print(F("I2C Slave Started on address: 0x"));
    Serial1.println(address, HEX);
}

void I2C::receiveEvent(int howMany) {
    _idx = 0;
    while (Wire.available() && _idx < sizeof(_buffer) - 1) {
        _buffer[_idx++] = Wire.read();
    }
    _buffer[_idx] = '\0';
    _newData = true;
}

// ฟังก์ชันนี้จะทำงานเมื่อ Master สั่ง Wire.requestFrom()
void I2C::requestEvent() {
    // เช็คว่า Master ขออ่าน Register อะไร (ซึ่ง Master เพิ่งส่งมาทาง receiveEvent)
    uint8_t reg = _buffer[0]; 
    uint8_t response[2] = {0, 0};

    if (reg == 0x00) { // ขออุณหภูมิ (Register 0x00)
        response[0] = (sensorTemp >> 8) & 0xFF; // High Byte
        response[1] = sensorTemp & 0xFF;        // Low Byte
    } 
    else if (reg == 0x01) { // ขอความชื้น (Register 0x01)
        response[0] = (sensorHumid >> 8) & 0xFF; 
        response[1] = sensorHumid & 0xFF;        
    }

    Wire.write(response, 2); // ส่งข้อมูล 2 Bytes กลับไปหา Master
}

void I2C::slave_loop() {
    if (_newData) {
        _newData = false;
        Serial1.printf("Master requested Register: 0x%02X\n", _buffer[0]); 
    }
}