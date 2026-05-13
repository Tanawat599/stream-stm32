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
    Serial1.println();
}
void I2C::loadConfig(const JsonObject& i2c) {
    Serial1.println(F("\n[I2C] Loading Config..."));

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

    // +++ เพิ่มสองบรรทัดนี้ +++
    _max_resp_ms = i2c["max_resp_ms"] | 1000;
    //_master_address = parseHex(i2c["master_address"] | "0x08");

    Serial1.printf("[I2C] Enabled: %d, Freq: %lu, Interval: %lu, Retry: %d, MaxResp: %lu ms\n",
                   _enabled, _frequency, _interval_ms, _max_retry, _max_resp_ms);

    _devices.clear();
    if (!_enabled) {
        Serial1.println(F("[I2C] Disabled. Skip device loading."));
        return;
    }

    JsonArray devices = i2c["devices"];
    if (devices.isNull()) {
        Serial1.println(F("[I2C] No devices found"));
        return;
    }

    for (JsonObject dev : devices) {
        I2C_Device d;
        d.name = dev["name"] | "unknown";

        if (dev["address"].is<const char*>())
            d.address = parseHex(dev["address"]);
        else
            d.address = dev["address"] | 0;

        JsonArray channels = dev["channels"];
        if (!channels.isNull()) {
            for (JsonObject ch : channels) {
                I2C_Channel c;
                c.name = ch["name"] | "ch";
                if (ch["register"].is<const char*>())
                    c.reg = parseHex(ch["register"]);
                else
                    c.reg = ch["register"] | 0;
                c.length     = ch["length"] | 2;
                c.byte_order = ch["byte_order"] | "AB";
                c.scale      = ch["scale"] | 1.0;
                d.channels.push_back(c);
            }
        }
        _devices.push_back(d);
    }

    Serial1.printf("[I2C] Devices Loaded: %d\n", _devices.size());
    for (const auto& dev : _devices) {
        Serial1.printf("  - %s [0x%02X] (%d channels)\n", dev.name.c_str(), dev.address, dev.channels.size());
        for (const auto& ch : dev.channels) {
            Serial1.printf("      > %s | Reg:0x%02X | Len:%d | Order:%s | Scale:%.2f\n",
                           ch.name.c_str(), ch.reg, ch.length, ch.byte_order.c_str(), ch.scale);
        }
    }
    Serial1.println(F("[I2C] Config Load Complete!\n"));
}
static bool isValidName(const char* name) {
    if (name == nullptr || name[0] == '\0') return false;
    for (int i = 0; i < 32; i++) {  
        if (name[i] == '\0') return true;
        if (!isprint(name[i])) return false;
    }
    return false;
}
void I2C::loadConfig(const I2CConfig& i2c_cfg) {
    resetInternalConfig();  // เรียก clear ก่อน

    _enabled     = i2c_cfg.enable;
    _frequency   = i2c_cfg.frequency;
    _interval_ms = i2c_cfg.interval_ms;
    _max_retry   = i2c_cfg.max_retry;

#ifdef I2C_CONFIG_HAS_MAX_RESP
    _max_resp_ms = i2c_cfg.max_resp_ms;
#else
    _max_resp_ms = 1000;
#endif

    Serial1.printf("[I2C] Enabled: %d, Freq: %lu, Interval: %lu, Retry: %d, MaxResp: %lu ms\n",
                   _enabled, _frequency, _interval_ms, _max_retry, _max_resp_ms);

    if (!_enabled) {
        Serial1.println(F("[I2C] Disabled. Skip device loading."));
        return;
    }
    const size_t deviceCount = sizeof(i2c_cfg.devices) / sizeof(i2c_cfg.devices[0]);
    for (size_t i = 0; i < deviceCount; i++) {
        const auto& dev_struct = i2c_cfg.devices[i];

        if (dev_struct.address == 0 || dev_struct.id == 0 || dev_struct.name[0] == '\0') {
            continue; 
        }

        I2C_Device d;
        d.name    = String(dev_struct.name);
        d.address = dev_struct.address;
        if (!isValidName(dev_struct.name)) {
            continue;
        }
        for (int j = 0; j < MAX_I2C_CHANNELS; j++) {
            const auto& ch_struct = dev_struct.channels[j];

            if (ch_struct.id == 0 || ch_struct.name[0] == '\0') {
                continue;
            }
            if (ch_struct.length < 1 || ch_struct.length > 4) {
                Serial1.printf("[I2C] Warning: channel '%s' has invalid length %d (ignored)\n",
                               ch_struct.name, ch_struct.length);
                continue;
            }
            String order = String(ch_struct.byte_order);
            if (order != "AB" && order != "BA" && order != "ABCD" && order != "DCBA") {
                Serial1.printf("[I2C] Warning: channel '%s' has invalid byte_order '%s' (using AB)\n",
                               ch_struct.name, order.c_str());
                order = "AB"; 
            }

            I2C_Channel c;
            c.name       = String(ch_struct.name);
            c.reg        = ch_struct.reg_addr;
            c.length     = ch_struct.length;
            c.byte_order = order;
            c.scale      = ch_struct.scale;

            d.channels.push_back(c);
        }

        if (!d.channels.empty()) {
            _devices.push_back(d);
        } else {
            Serial1.printf("[I2C] Warning: device '%s' has no valid channels, skipped\n", d.name.c_str());
        }
    }

    Serial1.printf("[I2C] Devices Loaded: %d\n", _devices.size());
    for (const auto& dev : _devices) {
        Serial1.printf("  - %s [0x%02X] (%d channels)\n", dev.name.c_str(), dev.address, dev.channels.size());
        for (const auto& ch : dev.channels) {
            Serial1.print("      > ");
            Serial1.print(ch.name);
            Serial1.print(" | Reg:0x");
            Serial1.print(ch.reg, HEX);
            Serial1.print(" | Len:");
            Serial1.print(ch.length);
            Serial1.print(" | Order:");
            Serial1.print(ch.byte_order);
            Serial1.print(" | Scale:");
            char buf[10];
            dtostrf(ch.scale, 4, 2, buf);
            Serial1.println(buf);
        }
    }
}

const char* I2C::master_loop() {
    static char payload[512];
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
    uint32_t startTime = millis();
    for (int r = 0; r < _max_retry && (millis() - startTime) < _max_resp_ms; r++) {
        Wire.beginTransmission(devAddr);
        Wire.write(regAddr);
        uint8_t error = Wire.endTransmission(false);

        if (error == 0) {
            uint8_t bytesReceived = Wire.requestFrom(devAddr, len);
            if (bytesReceived == len) {
                for (uint8_t i = 0; i < len; i++) {
                    buffer[i] = Wire.read();
                }
                // Success: print read data in same style as error messages
                Serial1.printf("[I2C] 0x%02X: reg 0x%02X -> read %d bytes: ", devAddr, regAddr, len);
                for (uint8_t i = 0; i < len; i++) {
                    Serial1.printf("%02X ", buffer[i]);
                }
                Serial1.printf("(retry %d/%d)\n", r+1, _max_retry);
                return true;
            } else {
                // concise error: expected vs got bytes
                Serial1.printf("[I2C] 0x%02X: reg 0x%02X -> got %d/%d bytes (retry %d/%d)\n",
                               devAddr, regAddr, bytesReceived, len, r+1, _max_retry);
            }
        } else {
            // compact error code
            Serial1.printf("[I2C] 0x%02X: reg 0x%02X -> err code %d (retry %d/%d)\n",
                           devAddr, regAddr, error, r+1, _max_retry);
            if (error == 4) {
                Serial1.printf("[I2C] Bus reset triggered for 0x%02X\n", devAddr);
                Wire.end();
                delay(10);
                Wire.begin();
                Wire.setClock(_frequency);
            }
        }
    }
    Serial1.printf("[I2C] FAILED to read 0x%02X:0x%02X after %d retries\n",
                   devAddr, regAddr, _max_retry);
    return false;
}
uint32_t I2C::processRawData(uint8_t* data, uint8_t len, String order) {
    if (len == 0 || len > 4) return 0;  
    
    uint32_t val = 0;
    if (order == "AB" || order == "ABCD") {
        for (uint8_t i = 0; i < len; i++) {
            val = (val << 8) | data[i];
        }
    }
    else if (order == "BA" || order == "DCBA") {
        for (uint8_t i = len; i > 0; i--) {
            val = (val << 8) | data[i - 1];
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
    _frequency = 100000;
    _interval_ms = 2000;
    _max_retry = 3;
    _max_resp_ms = 1000;
    _newData = false;
    _idx = 0;
    _currentRegister = 0;
    memset(_registers, 0, sizeof(_registers));
    memset(_buffer, 0, sizeof(_buffer));
}

