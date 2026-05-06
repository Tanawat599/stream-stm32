
#include <Arduino.h>
#include "mylib.h"
#include "config.h"

SDResourceManager sd(SD_MOSI, SD_MISO, SD_SCK, SD_CS);
LowSideSwitch ls;
Logger logger(&sd);
LoRaWan lorawan;
I2C i2cMaster;
MODBUS_RS485 modbus_rs485(&Serial2, RS485_DE_PIN, RS485_RE_PIN);

bool i2cEnabled = false;
bool ls_swEnabled = false;
bool oledEnabled = false;
bool ledEnabled = false;
bool analogEnabled = false;
bool modbus_rs485Enabled = false;

static char main_payload[256];
const char* i2c_payload = nullptr;
static char modbus_payload[128];

void setup() {
    Serial1.begin(115200);   
    delay(1000);
    Serial1.println("System Start");

    // ===== SD SPI =====
    SPI.setSCLK(SD_SCK);
    SPI.setMISO(SD_MISO);
    SPI.setMOSI(SD_MOSI);
    SPI.begin(); 

    if (!sd.begin()) {
        Serial1.println("SD init failed");
        while (1);
    }

    const char* file_name = sd.getConfig();

    File file = SD.open(file_name);
    if (!file) {
        Serial1.println("Open config failed");
        while (1);
    }

    StaticJsonDocument<4096> doc;
    auto err = deserializeJson(doc, file);
    file.close();

    if (err) {
        Serial1.println(err.c_str());
        while (1);
    }

    Serial1.println("JSON Loaded OK");

    JsonObject hw   = doc["hardware"];
    JsonObject lora = doc["lora"];

    // ===== FLAGS =====
    i2cEnabled          = hw["i2c"]["enable"] | false;
    modbus_rs485Enabled = hw["modbus_rs485"]["enable"] | false;
    oledEnabled = hw["oled"]["enable"] | hw["oled"]["enabled"] | false;
    ls_swEnabled = hw["ls_sw"]["enable"] | hw["ls_sw"]["enabled"] | false;
    ledEnabled   = hw["led"]["enable"] | hw["led"]["enabled"] | false;
    analogEnabled = hw["analog"]["enable"] | hw["analog"]["enabled"] | false;

    // ===== INIT =====
    if (i2cEnabled) {
        i2cMaster.loadConfig(hw["i2c"]);
        i2cMaster.master_begin();
    }

    if (modbus_rs485Enabled) {
        modbus_rs485.loadConfig(hw["modbus_rs485"]);
    }

    if (ls_swEnabled) {
        ls.loadConfig(hw["ls_sw"]);
        ls.begin();
    }

    // if (analogEnabled) {
    //     // Initialize analog components
    // }

    lorawan.loadConfig(lora);

    // ===== SWITCH SPI =====
    SPI.end();
    digitalWrite(SD_CS, HIGH);
    delay(100);

    SPI.setSCLK(PA5);
    SPI.setMISO(PA6);
    SPI.setMOSI(PA7);
    SPI.begin(); 
    
    lorawan.begin();

    Serial1.println("System Ready");
}
void loop() {

    memset(main_payload, 0, sizeof(main_payload));
    int main_len = 0;

    // ===== I2C =====
    // if (i2cEnabled) {
    //     const char* p = i2cMaster.master_loop();
    //     if (p && strlen(p) > 0) {
    //         main_len += snprintf(main_payload + main_len,
    //                              sizeof(main_payload) - main_len,
    //                              "I:%s|", p);
    //     }
    // }

// ===== MODBUS =====
    if (modbus_rs485Enabled) {
        char tmp[128];
        int len = 0;
        memset(tmp, 0, sizeof(tmp));

        modbus_rs485.FETCH_ALL();
        uint8_t count = modbus_rs485.GET_CH_COUNT();

        for (uint8_t i = 0; i < count; i++) {
            MODBUS_CH* ch = modbus_rs485.GET_CH(i);
            if (!ch) continue;

            uint32_t data = modbus_rs485.GET_DATA_BY_INDEX(i);

            float value = (ch->QTY == 2)
                ? *((float*)&data)
                : (float)data;

            // --- แก้ไขตรงนี้ ใช้ dtostrf แทน %f ---
            char valStr[16];
            dtostrf(value, 6, 2, valStr);

            len += snprintf(tmp + len,
                            sizeof(tmp) - len,
                            "%s:%s,", ch->NAME, valStr);
            // -------------------------------------
        }

        if (len > 0) {
            main_len += snprintf(main_payload + main_len,
                                 sizeof(main_payload) - main_len,
                                 "M:%s", tmp);
        }
    }

    // ===== SAFETY =====
    if (main_len >= sizeof(main_payload)) {
        main_payload[sizeof(main_payload) - 1] = '\0';
    }

    Serial1.print("PAYLOAD: ");
    Serial1.println(main_payload);

    lorawan.loop(main_payload);

    delay(5000);
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




// #include <Arduino.h>

// #define RS485_DE_PIN PB9
// #define RS485_RE_PIN PB8

// const uint8_t SLAVE_ID = 1;

// uint16_t calculate_modbus_crc(uint8_t* buf, uint8_t len) {
//     uint16_t crc_val = 0xFFFF;
//     for (uint8_t pos = 0; pos < len; pos++) {
//         crc_val ^= (uint16_t)buf[pos];
//         for (uint8_t i = 8; i != 0; i--) {
//             if ((crc_val & 0x0001) != 0) {
//                 crc_val >>= 1;
//                 crc_val ^= 0xA001;
//             } else {
//                 crc_val >>= 1;
//             }
//         }
//     }
//     return crc_val;
// }

// void setup() {
//     Serial1.begin(115200);
//     Serial2.begin(9600, SERIAL_8N1);
    
//     pinMode(RS485_DE_PIN, OUTPUT);
//     pinMode(RS485_RE_PIN, OUTPUT);
    
//     digitalWrite(RS485_DE_PIN, LOW); 
//     digitalWrite(RS485_RE_PIN, LOW); 
    
//     Serial1.println("Modbus Slave Ready on USART2. Waiting for Master...");
// }

// void loop() {
//     if (Serial2.available() >= 8) {
//         uint8_t req[8];
//         Serial2.readBytes(req, 8);

//         uint16_t recv_crc = (req[7] << 8) | req[6];
//         uint16_t comp_crc = calculate_modbus_crc(req, 6);

//         if (req[0] == SLAVE_ID && recv_crc == comp_crc) {
//             uint8_t func_code = req[1];
//             uint16_t start_addr = (req[2] << 8) | req[3];
//             uint16_t qty = (req[4] << 8) | req[5];
            
//             uint8_t resp[64];
//             resp[0] = SLAVE_ID;
//             resp[1] = func_code;
//             resp[2] = qty * 2;
            
//             uint8_t byte_count = 3;

//             // จำลองค่าส่งกลับ
//             if (func_code == 3 && start_addr == 0) {
//                 uint16_t mock_temp = 2550;
//                 resp[byte_count++] = mock_temp >> 8;
//                 resp[byte_count++] = mock_temp & 0xFF;
//             } else if (func_code == 4 && start_addr == 1) {
//                 float mock_hum = 65.43;
//                 uint32_t raw_hum;
//                 memcpy(&raw_hum, &mock_hum, sizeof(raw_hum));
                
//                 resp[byte_count++] = (raw_hum >> 24) & 0xFF;
//                 resp[byte_count++] = (raw_hum >> 16) & 0xFF;
//                 resp[byte_count++] = (raw_hum >> 8) & 0xFF;
//                 resp[byte_count++] = raw_hum & 0xFF;
//             } else {
//                 for (int i = 0; i < qty * 2; i++) resp[byte_count++] = 0;
//             }

//             uint16_t res_crc = calculate_modbus_crc(resp, byte_count);
//             resp[byte_count++] = res_crc & 0xFF;
//             resp[byte_count++] = res_crc >> 8;

//             // 🌟 ให้ Slave รอสักครู่ เพื่อให้ Master สลับกลับมารับข้อมูลทัน
//             delay(5); 

//             digitalWrite(RS485_DE_PIN, HIGH); 
//             digitalWrite(RS485_RE_PIN, HIGH); 
            
//             Serial2.write(resp, byte_count);
//             Serial2.flush();
//             delay(2); // ป้องกันไบต์สุดท้ายหล่นหายจากฝั่ง Slave
            
//             digitalWrite(RS485_DE_PIN, LOW);
//             digitalWrite(RS485_RE_PIN, LOW);
            
//             Serial1.println("Received request and sent response to Master.");
//         }
//     }
// }