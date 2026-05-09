#include "mylib.h"

// Initialize static members
volatile bool I2C::_newData = false;
char I2C::_buffer[32];
volatile int I2C::_idx = 0;

volatile uint8_t I2C::_currentRegister = 0;
uint8_t I2C::_registers[16] = {0}; 

static uint16_t sensorTemp = 255; 
static uint16_t sensorHumid = 600;
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
void I2C::loadConfig(const JsonObject& i2c) {
    Serial1.println(F("\n[I2C] Loading Config..."));

    // ===== VALIDATE =====
    if (i2c.isNull()) {
        Serial1.println(F("[I2C] ERROR: Config is NULL"));
        _enabled = false;
        return;
    }

    // ===== BASIC CONFIG =====
    _enabled     = i2c["enable"] | false;
    _frequency   = i2c["frequency"] | 100000;
    _interval_ms = i2c["interval_ms"] | 2000;
    _max_retry   = i2c["max_retry"] | 3;

    Serial1.printf("[I2C] Enabled: %d, Freq: %lu, Interval: %lu, Retry: %d\n",
                   _enabled, _frequency, _interval_ms, _max_retry);

    // ===== CLEAR OLD DATA =====
    _devices.clear();

    if (!_enabled) {
        Serial1.println(F("[I2C] Disabled. Skip device loading."));
        return;
    }

    // ===== DEVICES =====
    JsonArray devices = i2c["devices"];
    if (devices.isNull()) {
        Serial1.println(F("[I2C] No devices found"));
        return;
    }

    for (JsonObject dev : devices) {
        I2C_Device d;

        // ---- NAME ----
        d.name = dev["name"] | "unknown";

        // ---- ADDRESS ----
        if (dev["address"].is<const char*>()) {
            d.address = parseHex(dev["address"]);   // "0x40"
        } else {
            d.address = dev["address"] | 0;
        }

        // ---- CHANNELS ----
        JsonArray channels = dev["channels"];
        if (channels.isNull()) continue;

        for (JsonObject ch : channels) {
            I2C_Channel c;

            c.name = ch["name"] | "ch";

            // register
            if (ch["register"].is<const char*>()) {
                c.reg = parseHex(ch["register"]);
            } else {
                c.reg = ch["register"] | 0;
            }

            c.length     = ch["length"] | 2;
            c.byte_order = ch["byte_order"] | "AB";
            c.scale      = ch["scale"] | 1.0;

            d.channels.push_back(c);
        }

        _devices.push_back(d);
    }

    // ===== DEBUG PRINT =====
    Serial1.printf("[I2C] Devices Loaded: %d\n", _devices.size());

    for (const auto& dev : _devices) {
        Serial1.printf("  - %s [0x%02X] (%d channels)\n",
                       dev.name.c_str(),
                       dev.address,
                       dev.channels.size());

        for (const auto& ch : dev.channels) {
            Serial1.printf("      > %s | Reg:0x%02X | Len:%d | Order:%s | Scale:%.2f\n",
                           ch.name.c_str(),
                           ch.reg,
                           ch.length,
                           ch.byte_order.c_str(),
                           ch.scale);
        }
    }

    Serial1.println(F("[I2C] Config Load Complete!\n"));
}
void I2C::loadConfig(const I2CConfig& i2c_cfg) {
    Serial1.println(F("\n[I2C] Loading Config from EEPROM (Struct)..."));

    // ===== 1. RESET OLD DATA =====
    resetInternalConfig();

    // ===== 2. BASIC CONFIG =====
    // ดึงค่าจาก Struct มาเก็บใน Member Variables ของ Class
    _enabled     = i2c_cfg.enable;
    _frequency   = i2c_cfg.frequency;
    _interval_ms = i2c_cfg.interval_ms;
    _max_retry   = i2c_cfg.max_retry;

    Serial1.printf("[I2C] Enabled: %d, Freq: %lu, Interval: %lu, Retry: %d\n",
                   _enabled, _frequency, _interval_ms, _max_retry);

    // ถ้าไม่ได้ Enable ก็ไม่ต้องโหลด Devices ต่อ
    if (!_enabled) {
        Serial1.println(F("[I2C] Disabled. Skip device loading."));
        return;
    }

    // ===== 3. DEVICES & CHANNELS =====
    // วนลูปตามจำนวนอุปกรณ์สูงสุดที่กำหนดไว้ใน Struct (เช่น devices[1])
    for (int i = 0; i < 1; i++) { 
        const auto& dev_struct = i2c_cfg.devices[i];

        // ตรวจสอบว่า Address มีค่าหรือไม่ (ถ้าเป็น 0 แสดงว่าเป็นช่องว่าง)
        if (dev_struct.address == 0) continue;

        I2C_Device d;
        d.name    = String(dev_struct.name);
        d.address = dev_struct.address;

        // วนลูปโหลด Channels (เช่น channels[2])
        for (int j = 0; j < 2; j++) {
            const auto& ch_struct = dev_struct.channels[j];
            
            // เช็คว่า channel นี้มีการใช้งานหรือไม่ (ดูจากชื่อ หรือ ID)
            if (strlen(ch_struct.name) == 0) continue;

            I2C_Channel c;
            c.name       = String(ch_struct.name);
            c.reg        = ch_struct.reg_addr;
            c.length     = ch_struct.length;
            c.byte_order = String(ch_struct.byte_order);
            c.scale      = ch_struct.scale;

            d.channels.push_back(c);
        }

        _devices.push_back(d);
    }

    // ===== 4. DEBUG PRINT (Optional) =====
    Serial1.printf("[I2C] Devices Loaded: %d\n", _devices.size());
    for (const auto& dev : _devices) {
        Serial1.printf("  - %s [0x%02X] (%d channels)\n",
                       dev.name.c_str(), dev.address, dev.channels.size());
        for (const auto& ch : dev.channels) {
            Serial1.printf("      > %s | Reg:0x%02X | Len:%d | Order:%s | Scale:%.2f\n",
                           ch.name.c_str(), ch.reg, ch.length, 
                           ch.byte_order.c_str(), ch.scale);
        }
    }

    Serial1.println(F("[I2C] EEPROM Config Load Complete!\n"));
}

const char* I2C::master_loop() {
    static char payload[256];
    memset(payload, 0, sizeof(payload));
    int len = 0;

    if (!_enabled || _devices.empty()) return "";

    if (millis() - _lastPoll < _interval_ms) {
        return ""; 
    }

    _lastPoll = millis();

    for (auto& dev : _devices) {
        int w = snprintf(payload + len, sizeof(payload) - len,
                         ">>> %s [0x%02X]\n", dev.name.c_str(), dev.address);
        
        if (w < 0 || w >= (sizeof(payload) - len)) break;
        len += w;

        for (auto& ch : dev.channels) {
            uint8_t raw[4] = {0};
            if (readRegister(dev.address, ch.reg, raw, ch.length)) {
                uint32_t rawVal = processRawData(raw, ch.length, ch.byte_order);
                float val = rawVal * ch.scale;
                
                char valStr[16];
                dtostrf(val, 6, 2, valStr);

                w = snprintf(payload + len, sizeof(payload) - len,
                             " - %s: %s\n", ch.name.c_str(), valStr);
            } else {
                w = snprintf(payload + len, sizeof(payload) - len,
                             " - %s: FAIL\n", ch.name.c_str());
            }

            if (w < 0 || w >= (sizeof(payload) - len)) break;
            len += w;
        }
    }

    return payload;
}

bool I2C::readRegister(uint8_t devAddr, uint8_t regAddr, uint8_t* buffer, uint8_t len) {
    for (int r = 0; r < _max_retry; r++) {
        Wire.beginTransmission(devAddr);
        Wire.write(regAddr);
        
        uint8_t error = Wire.endTransmission(false); 
        
        if (error == 0) { 
            uint8_t bytesReceived = Wire.requestFrom((uint8_t)devAddr, (uint8_t)len);
            
            if (bytesReceived == len) {
                for (uint8_t i = 0; i < len; i++) {
                    buffer[i] = Wire.read();
                }
                return true; 
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
            

            if (error == 4) {
                Serial1.print(F("[Resetting I2C Bus...] "));
                Wire.end();               
                delay(10);
                Wire.begin();              
                Wire.setClock(_frequency); 
            }
        }
        
        delay(50); 
    }
    return false; 
}
uint32_t I2C::processRawData(uint8_t* data, uint8_t len, String order) {
    uint32_t val = 0;
    
    if (order == "AB" || order == "ABCD") { 
        for (int i = 0; i < len; i++) {
            val = (val << 8) | data[i];
        }
    } else if (order == "BA" || order == "DCBA") { 
        for (int i = len - 1; i >= 0; i--) {
            val = (val << 8) | data[i];
        }
    }
    return val;
}

uint8_t I2C::parseHex(const char* str) {
    if (!str) return 0;
    return (uint8_t)strtol(str, NULL, 0);
}

// ==========================================
// ===== Slave Logic (Sensor Simulator) =====
// ==========================================



void I2C::slave_begin(uint8_t address) {
    Wire.begin(address);
    Wire.onReceive(receiveEvent); 
    Wire.onRequest(requestEvent); 
    
    Serial1.print(F("I2C Slave Started on address: 0x"));
    Serial1.println(address, HEX);
}

void I2C::requestEvent() {
    _registers[0x00] = (sensorTemp >> 8) & 0xFF; // Temp High
    _registers[0x01] = sensorTemp & 0xFF;        // Temp Low
    _registers[0x02] = (sensorHumid >> 8) & 0xFF; // Humid High
    _registers[0x03] = sensorHumid & 0xFF;        // Humid Low

    if (_currentRegister >= sizeof(_registers)) {
        uint8_t err = 0xFF;
        Wire.write(&err, 1);
        return;
    }

    uint8_t bytesToSend = sizeof(_registers) - _currentRegister;

    Wire.write(&_registers[_currentRegister], bytesToSend);
}

void I2C::receiveEvent(int howMany) {
    if (howMany < 1) return;

    _currentRegister = Wire.read();
    howMany--;

    while (howMany > 0 && Wire.available()) {
        if (_currentRegister < sizeof(_registers)) {
            _registers[_currentRegister] = Wire.read();
            _currentRegister++; 
        } else {
            Wire.read(); 
        }
        howMany--;
    }

    _newData = true;
}

void I2C::slave_loop() {
    if (_newData) {
        _newData = false;
        Serial1.printf("Master requested Register: 0x%02X\n", _buffer[0]); 
    }
}

void I2C::resetInternalConfig() {
    _devices.clear();
    
    _enabled = false;
    _lastPoll = 0;
}