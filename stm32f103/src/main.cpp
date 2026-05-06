
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

    // ===== SD INIT =====
    if (!sd.begin()) {
        Serial1.println("SD init failed");
        while (1);
    }
    Serial1.println("SD init OK");
    sd.listFiles(Serial1);
    const char* file_name = sd.getConfig();
    Serial1.print("Config file: ");
    Serial1.println(file_name);

    // ===== Check Hardwares =====
    ls_swEnabled = sd.checkHardwares(file_name, "ls_sw");
    oledEnabled = sd.checkHardwares(file_name, "oled");
    i2cEnabled = sd.checkHardwares(file_name, "i2c");
    modbus_rs485Enabled = sd.checkHardwares(file_name, "modbus_rs485");
    analogEnabled = sd.checkHardwares(file_name, "analog");
    ledEnabled = sd.checkHardwares(file_name, "led");

    // ===== Load Configurations =====
    if (ls_swEnabled) {
        if (ls.loadConfig(sd, file_name)) {
            ls.begin();
        } else {
            Serial1.println("LS Switch config failed"); 
            ls_swEnabled = false;
        }
    }
    if (oledEnabled) {
        OLED oled;
        oled.begin();
        oled.loadConfig(sd, file_name);
    }
    if (modbus_rs485Enabled) {
        if (modbus_rs485.loadConfig(sd, file_name)) {
            Serial1.println("MODBUS RS485 config loaded");
        } else {
            Serial1.println("MODBUS RS485 config failed");
            modbus_rs485Enabled = false;
        }
    }
    if (i2cEnabled) {
        
        i2cMaster.loadConfig(sd, file_name);
        i2cMaster.master_begin();
    }
    if (ledEnabled) {
        pinMode(LED_BUILTIN, OUTPUT);
    }
    // ===== LoRaWan Configurations =====
    pinMode(SD_CS, OUTPUT);  digitalWrite(SD_CS, HIGH);
    pinMode(LORA_SS_PIN, OUTPUT); digitalWrite(LORA_SS_PIN, HIGH);
    Serial1.println("--- Reading SD Card Config ---");

    SPI.setSCLK(SD_SCK);
    SPI.setMISO(SD_MISO);
    SPI.setMOSI(SD_MOSI);
    SPI.begin(); 

    if (sd.begin()) {
        logger.logMsg("SYSTEM", "SD initialized");
        lorawan.loadConfig(sd, file_name);
        Serial1.println("SD Config Loaded.");
    } else {
        Serial1.println("SD init failed!");
    }

    Serial1.println("--- Starting LoRaWAN ---");
    
    SPI.end(); 
    digitalWrite(SD_CS, HIGH); 
    delay(100);

    SPI.setSCLK(PA5);
    SPI.setMISO(PA6);
    SPI.setMOSI(PA7);
    SPI.begin(); 
    
    lorawan.begin();

}

void loop() {
    // ===== Main Payload Loop (uplink) =====
    if( i2cEnabled ) {
        
        i2c_payload = i2cMaster.master_loop();
        Serial1.print("I2C Payload: ");
        Serial1.println(i2c_payload);
    }
    if (modbus_rs485Enabled) {
        
        int payload_len = 0;

        memset(modbus_payload, 0, sizeof(modbus_payload));

        modbus_rs485.FETCH_ALL();
        uint8_t count = modbus_rs485.GET_CH_COUNT();

        for (uint8_t i = 0; i < count; i++) {
            MODBUS_CH* ch = modbus_rs485.GET_CH(i);
            if (ch == nullptr) continue;

            uint32_t data = modbus_rs485.GET_DATA_BY_INDEX(i);

            float value;

            if (ch->QTY == 2) {
                memcpy(&value, &data, sizeof(value));
            } else {
                value = data;
            }

            // debug
            Serial1.print("MODBUS ");
            Serial1.print(ch->NAME);
            Serial1.print(": ");
            Serial1.println(value);

            // build payload
            payload_len += snprintf(modbus_payload + payload_len,
                                    sizeof(modbus_payload) - payload_len,
                                    "%s:%.2f,", ch->NAME, value);
        }

        Serial1.print("MODBUS Payload: ");
        Serial1.println(modbus_payload);
    }
    memset(main_payload, 0, sizeof(main_payload));
    int main_len = 0;
    if (i2cEnabled && i2c_payload != nullptr) {
    main_len += snprintf(main_payload + main_len,
                         sizeof(main_payload) - main_len,
                         "I:%s|", i2c_payload);
    }

    // ===== MODBUS =====
    if (modbus_rs485Enabled) {
        main_len += snprintf(main_payload + main_len,
                            sizeof(main_payload) - main_len,
                            "M:%s", modbus_payload);
    }
    if (main_len >= sizeof(main_payload)) {
        main_payload[sizeof(main_payload) - 1] = '\0';
    }

    Serial1.print("MAIN Payload: ");
    Serial1.println(main_payload);
    lorawan.loop(main_payload);

    delay(5000);
    // ===== Main Payload Loop (downlink) =====


}
