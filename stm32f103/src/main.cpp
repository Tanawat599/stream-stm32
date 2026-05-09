#include <Arduino.h>
#include "mylib.h"
#include "config.h"
#include "device_config.h" 

SDResourceManager sd(SD_MOSI, SD_MISO, SD_SCK, SD_CS);
LowSideSwitch ls;
Logger logger(&sd);
LoRaWan lorawan;
I2C i2cMaster;
MODBUS_RS485 modbus_rs485(&Serial2, RS485_DE_PIN, RS485_RE_PIN);
OLED oled;
MySHTC3 sht(&Wire, PB7, PB6);

ConfigManager cfgMgr;
SerialCLI cli;

bool i2cEnabled = false;
bool ls_swEnabled = false;
bool oledEnabled = false;
bool ledEnabled = false;
bool analogEnabled = false;
bool modbus_rs485Enabled = false;
bool sht3Enabled = false;

String displayText = "";
static char main_payload[256];
static char modbus_payload[128];
const char* downlink_payload = nullptr;
bool isLogging = true;
unsigned long lastMainLoop = 0; 

float readBatteryVoltage() {

    int adc = analogRead(VBATT_PIN);
    float v_adc = (adc * ADC_VREF) / ADC_RES;
    float v_batt = v_adc / VBATT_DIVIDER;

    return v_batt;
}

float readCurrent_mA() {

    int adc = analogRead(CURRENT_PIN);
    float voltage = (adc * ADC_VREF) / ADC_RES;
    // I = V/R
    float current_A = voltage / SHUNT_RESISTOR;
    float current_mA = current_A * 1000.0;

    return current_mA;
}

String getChargeStatus() {

    bool chrg = digitalRead(CHRG_PIN);
    bool done = digitalRead(DONE_PIN);

    // active low
    if (!chrg && done) {
        return "Charging";
    }

    if (chrg && !done) {
        return "Charge Full";
    }

    return "Idle";
}

void setup() {
    Serial1.begin(115200);   
    delay(1000);
    
    cfgMgr.begin();
    cli.begin(Serial1, cfgMgr);
    
    Serial1.println("\n[SYSTEM] Booting...");

    DeviceConfig& cfg = cfgMgr.get();

    // ===== Button & ADC Setup =====
    pinMode(BUTTON_PIN, INPUT);
    pinMode(CHRG_PIN, INPUT_PULLUP);
    pinMode(DONE_PIN, INPUT_PULLUP);
    analogReadResolution(12);

    // ===== SD SPI Init =====
    SPI.setSCLK(SD_SCK);
    SPI.setMISO(SD_MISO);
    SPI.setMOSI(SD_MOSI);
    SPI.begin(); 

    bool sd_ready = sd.begin();
    if (!sd_ready) {
        Serial1.println("[WARN] SD init failed, forced to use EEPROM.");
        cfg.device.use_sd_config = false;
    }

    // =========================================================
    //                BRANCH CONFIG SOURCE
    // =========================================================
    if (cfg.device.use_sd_config) {
        Serial1.println("[CONFIG] Source: SD Card (JSON)");

        const char* file_name = sd.getConfig();
        File file = SD.open(file_name);
        if (!file) {
            Serial1.println("[ERROR] Open config.json failed. System Halted.");
            while (1) cli.update(); 
        }

        StaticJsonDocument<4096> doc;
        auto err = deserializeJson(doc, file);
        file.close();

        if (err) {
            Serial1.println("[ERROR] JSON parse failed.");
            while (1) cli.update(); 
        }

        JsonObject hw   = doc["hardware"];
        JsonObject lora = doc["lora"];

        i2cEnabled          = hw["i2c"]["enable"] | false;
        modbus_rs485Enabled = hw["modbus_rs485"]["enable"] | false;
        oledEnabled         = hw["oled"]["enable"] | false;
        ls_swEnabled        = hw["ls_sw"]["enable"] | false;
        ledEnabled          = hw["led"]["enable"] | false;
        analogEnabled = hw["analog"]["enable"] | hw["analog"]["enabled"] | false;
        sht3Enabled         = hw["sht3"]["enable"] | false;

        if (i2cEnabled) {
            i2cMaster.loadConfig((JsonObject)doc["hardware"]["i2c"]);
            i2cMaster.master_begin();
        }
        if (modbus_rs485Enabled) {
            JsonObject modbus_json = hw["modbus_rs485"].as<JsonObject>();
            modbus_rs485.loadConfigFromJson(modbus_json);
        }
        if (ls_swEnabled) {
            JsonObject ls_json = hw["ls_sw"].as<JsonObject>();
            ls.loadConfigFromJson(ls_json);
            ls.begin();
        }
        if (oledEnabled) {
            oled.begin();
            oled.clear();
            oled.setFont(u8g2_font_5x7_tr);
            oled.print("Booting...", 0, 10);
            oled.update();
        }
        if (sht3Enabled) {
            if (!sht.begin()) {
                Serial1.println("SHTC3 init failed");
                while (1);
            }
            Serial1.println("SHTC3 init success");
    }
        lorawan.loadConfig(lora);

    } 
    else {
        Serial1.println("[CONFIG] Source: EEPROM (Struct)");

        i2cEnabled          = cfg.hardware.i2c.enable;
        modbus_rs485Enabled = cfg.hardware.modbus_rs485.enable;
        oledEnabled         = cfg.hardware.oled.enabled;
        ls_swEnabled        = cfg.hardware.ls_sw.enable;
        ledEnabled          = cfg.hardware.led.active_low;
        sht3Enabled         = cfg.hardware.sht3.enable;

        if (i2cEnabled) {
            i2cMaster.loadConfig((const I2CConfig&)cfg.hardware.i2c);
            i2cMaster.master_begin();
        }
        if (modbus_rs485Enabled) modbus_rs485.loadConfigFromStruct(cfg.hardware);
        if (ls_swEnabled) {
            ls.loadConfigFromStruct(cfg.hardware);
            ls.begin();
        }
        lorawan.loadConfigFromStruct(cfg.lora);
        
    }
    // =========================================================

    // ===== INIT COMMON HARDWARE =====
    if (oledEnabled) {
        oled.begin();
        oled.clear();
        oled.setFont(u8g2_font_5x7_tr);
        oled.print("Booting...", 0, 10);
        oled.update();
    }
    if (ledEnabled) {
        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, LOW);
    }
    if (sht3Enabled && sht.begin()) {
        Serial1.println("SHTC3 init success");
    }

    // ===== SWITCH SPI FOR LORA =====
    SPI.end();
    digitalWrite(SD_CS, HIGH);
    delay(100);

    SPI.setSCLK(PA5);
    SPI.setMISO(PA6);
    SPI.setMOSI(PA7);
    SPI.begin(); 
    
    lorawan.begin();

    Serial1.println("[SYSTEM] Ready. Press ANY KEY to enter CLI.");
    cli.printPrompt(); // พิมพ์ Prompt ขึ้นมารอรับคำสั่ง
}
void loop() {
    // Detect serial input to pause logging and enter CLI mode
    if (isLogging && Serial1.available() > 0) {
        isLogging = false; 
        delay(10); 
        
        // Clear remaining characters in serial buffer to prevent ghost commands
        while(Serial1.available()) { Serial1.read(); } 

        Serial1.println("\n\x1b[33m=========================================\x1b[0m");
        Serial1.println("\x1b[1m    LOGGING PAUSED - ENTERED CLI MODE    \x1b[0m");
        Serial1.println("\x1b[33m=========================================\x1b[0m");
        Serial1.println("Type 'resume' or 'exit' to start logging again.\n");
        cli.printPrompt();
    }

    // Process CLI commands (Sensors & LoRa are paused)
    if (!isLogging) {
        cli.update();
    } 
    // Normal operation mode (Execute every 100ms without blocking)
    else {
        if (millis() - lastMainLoop >= 100) {
            lastMainLoop = millis();

            memset(main_payload, 0, sizeof(main_payload));
            StaticJsonDocument<256> payloadDoc; 

            // ===== Read I2C Devices =====
            if (i2cEnabled) {
                displayText += " I2C ";
                const char* p = i2cMaster.master_loop();
                if (p && strlen(p) > 0) payloadDoc["i2c"] = p; 
            }

            // ===== Read Modbus RS485 =====
            if (modbus_rs485Enabled) {
                displayText += " modbus ";
                JsonObject modbusData = payloadDoc.createNestedObject("modbus");

                modbus_rs485.FETCH_ALL();
                uint8_t count = modbus_rs485.GET_CH_COUNT();

                for (uint8_t i = 0; i < count; i++) {
                    MODBUS_CH* ch = modbus_rs485.GET_CH(i);
                    if (!ch) continue;

                    uint32_t data = modbus_rs485.GET_DATA_BY_INDEX(i);
                    float value = (ch->QTY == 2) ? *((float*)&data) : (float)data;
                    modbusData[ch->NAME] = serialized(String(value, 2));
                }
            }

            // ===== Read SHTC3 Sensor =====
            if (sht3Enabled && sht.read()) {
                displayText += " SHT3 ";
                JsonObject shtData = payloadDoc.createNestedObject("sht3");
                shtData["temp"] = serialized(String(sht.getTemperature(), 2));
                shtData["hum"]  = serialized(String(sht.getHumidity(), 2));
            }

            // ===== Read Battery & Charging Status =====
            JsonObject batData = payloadDoc.createNestedObject("bat");
            batData["v"]  = serialized(String(readBatteryVoltage(), 2));
            batData["mA"] = serialized(String(readCurrent_mA(), 1));
            batData["st"] = getChargeStatus();

            // Serialize data for LoRa transmission
            serializeJson(payloadDoc, main_payload, sizeof(main_payload));

            // ===== LoRaWAN Process (Uplink & Downlink) =====
            lorawan.loop(main_payload);
            
            if (lorawan.available()) {
                downlink_payload = lorawan.getDownlink();
                Serial1.print("DOWNLINK HEX: ");
                Serial1.println(downlink_payload);

                int len = strlen(downlink_payload);
                if (downlink_payload != nullptr && len >= 2) {
                    
                    // Parse command code (First byte)
                    char cmdStr[3] = {downlink_payload[0], downlink_payload[1], '\0'};
                    uint8_t command = strtol(cmdStr, NULL, 16); 

                    switch (command) {
                        case 0x01: {  // CMD 01: Control LED
                            if (len >= 4 && ledEnabled) { 
                                char stateStr[3] = {downlink_payload[2], downlink_payload[3], '\0'};
                                uint8_t state = strtol(stateStr, NULL, 16);
                                digitalWrite(LED_PIN, state ? HIGH : LOW);
                                Serial1.printf("ACTION: LED -> %s\n", state ? "ON" : "OFF");
                            }
                            break;
                        }
                        case 0x02: {  // CMD 02: Control Low-Side Switch
                            if (len >= 6 && ls_swEnabled) { 
                                char chStr[3]   = {downlink_payload[2], downlink_payload[3], '\0'};
                                char stateStr[3] = {downlink_payload[4], downlink_payload[5], '\0'};
                                uint8_t channel = strtol(chStr, NULL, 16);
                                uint8_t state   = strtol(stateStr, NULL, 16);
                                Serial1.printf("ACTION: LS Switch CH:%d -> %s\n", channel, state ? "ON" : "OFF");
                            }
                            break;
                        }
                        case 0x03: {  // CMD 03: Display Text on OLED
                            if (len > 2 && oledEnabled) {
                                char textBuffer[32] = {0}; 
                                int textIdx = 0;
                                
                                // Parse Hex string to ASCII characters
                                for (int i = 2; i < len && textIdx < 31; i += 2) {
                                    char hexChar[3] = {downlink_payload[i], downlink_payload[i+1], '\0'};
                                    textBuffer[textIdx++] = (char)strtol(hexChar, NULL, 16);
                                }
                                oled.clear();
                                oled.setFont(u8g2_font_5x7_tr); 
                                oled.print(textBuffer, 0, 10);
                                oled.update();
                                Serial1.printf("ACTION: OLED -> %s\n", textBuffer);
                            }
                            break;
                        }
                        default:
                            Serial1.printf("UNKNOWN COMMAND: 0x%02X\n", command);
                            break;
                    }
                }
            }

            // ===== Update OLED UI =====
            if (oledEnabled) {
                oled.updateDisplay(displayText.c_str(), downlink_payload ? downlink_payload : "ok");
            }
            displayText = ""; // Clear text for the next cycle
        }
    }
}