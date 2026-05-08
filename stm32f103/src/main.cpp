#include <Arduino.h>
#include "mylib.h"
#include "config.h"

SDResourceManager sd(SD_MOSI, SD_MISO, SD_SCK, SD_CS);
LowSideSwitch ls;
Logger logger(&sd);
LoRaWan lorawan;
I2C i2cMaster;
MODBUS_RS485 modbus_rs485(&Serial2, RS485_DE_PIN, RS485_RE_PIN);
OLED oled;
MySHTC3 sht(&Wire, PB7, PB6);

bool i2cEnabled = false;
bool ls_swEnabled = false;
bool oledEnabled = false;
bool ledEnabled = false;
bool analogEnabled = false;
bool modbus_rs485Enabled = false;
bool sht3Enabled = false;

String displayText = "";

static char main_payload[256];
const char* i2c_payload = nullptr;
static char modbus_payload[128];

const char* downlink_payload = nullptr;



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
    Serial1.println("System Start");
    // ===== Button Setup =====
    pinMode(BUTTON_PIN, INPUT);

    // ===== Battery ADC Setup =====
    pinMode(BUTTON_PIN, INPUT);

    pinMode(CHRG_PIN, INPUT_PULLUP);
    pinMode(DONE_PIN, INPUT_PULLUP);

    analogReadResolution(12);

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
    sht3Enabled = hw["sht3"]["enable"] | hw["sht3"]["enabled"] | false;

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

    if (sht3Enabled) {
        if (!sht.begin()) {
            Serial1.println("SHTC3 init failed");
            while (1);
        }
        Serial1.println("SHTC3 init success");
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
    delay(1000);
}
void loop() {
    memset(main_payload, 0, sizeof(main_payload));
    
    StaticJsonDocument<256> payloadDoc; 

    // ===== I2C =====
    if (i2cEnabled) {
        displayText += " I2C ";
        const char* p = i2cMaster.master_loop();
        if (p && strlen(p) > 0) {
            payloadDoc["i2c"] = p; 
        }
    }

    // ===== MODBUS =====
    if (modbus_rs485Enabled) {
        displayText += " modbus ";
        
        JsonObject modbusData = payloadDoc.createNestedObject("modbus");

        modbus_rs485.FETCH_ALL();
        uint8_t count = modbus_rs485.GET_CH_COUNT();

        for (uint8_t i = 0; i < count; i++) {
            MODBUS_CH* ch = modbus_rs485.GET_CH(i);
            if (!ch) continue;

            uint32_t data = modbus_rs485.GET_DATA_BY_INDEX(i);

            float value = (ch->QTY == 2)
                ? *((float*)&data)
                : (float)data;

            modbusData[ch->NAME] = serialized(String(value, 2));
        }
    }

    // ===== SHT3 =====
    if (sht3Enabled && sht.read()) {
        displayText += " SHT3 ";
        JsonObject shtData = payloadDoc.createNestedObject("sht3");
        shtData["temp"] = serialized(String(sht.getTemperature(), 2));
        shtData["hum"] = serialized(String(sht.getHumidity(), 2));
    }


    // ===== BATTERY & CHARGING =====
    JsonObject batData = payloadDoc.createNestedObject("bat");
    batData["v"] = serialized(String(readBatteryVoltage(), 2));
    batData["mA"] = serialized(String(readCurrent_mA(), 1));
    batData["st"] = getChargeStatus();

    serializeJson(payloadDoc, main_payload, sizeof(main_payload));

    // Serial1.print("PAYLOAD: ");
    // Serial1.println(main_payload);

    lorawan.loop(main_payload);
    if (lorawan.available()) {
        const char* downlink_payload = lorawan.getDownlink();
        Serial1.print("DOWNLINK HEX: ");
        Serial1.println(downlink_payload);

        int len = strlen(downlink_payload);
        
        if (downlink_payload != nullptr && len >= 2) {
            
            char cmdStr[3] = {downlink_payload[0], downlink_payload[1], '\0'};
            uint8_t command = strtol(cmdStr, NULL, 16); 

            switch (command) {

                case 0x01: {  
                    if (len >= 4 && ledEnabled) { 
                        char stateStr[3] = {downlink_payload[2], downlink_payload[3], '\0'};
                        uint8_t state = strtol(stateStr, NULL, 16);
                        
                        digitalWrite(LED_PIN, state ? HIGH : LOW);
                        Serial1.print("ACTION: LED -> ");
                        Serial1.println(state ? "ON" : "OFF");
                    }
                    break;
                }

                case 0x02: {  
                    if (len >= 6 && ls_swEnabled) { 
                        char chStr[3] = {downlink_payload[2], downlink_payload[3], '\0'};
                        char stateStr[3] = {downlink_payload[4], downlink_payload[5], '\0'};
                        
                        uint8_t channel = strtol(chStr, NULL, 16);
                        uint8_t state = strtol(stateStr, NULL, 16);
                        
                        Serial1.print("ACTION: LS Switch CH:");
                        Serial1.print(channel);
                        Serial1.print(" -> ");
                        Serial1.println(state ? "ON" : "OFF");
                    }
                    break;
                }

                case 0x03: {  
                    if (len > 2 && oledEnabled) {
                        char textBuffer[32] = {0}; 
                        int textIdx = 0;

                        for (int i = 2; i < len && textIdx < 31; i += 2) {
                            char hexChar[3] = {downlink_payload[i], downlink_payload[i+1], '\0'};
                            textBuffer[textIdx++] = (char)strtol(hexChar, NULL, 16);
                        }
                        
                        oled.clear();
                        oled.setFont(u8g2_font_5x7_tr); 
                        oled.print(textBuffer, 0, 10);
                        oled.update();

                        Serial1.print("ACTION: OLED Display -> ");
                        Serial1.println(textBuffer);
                    }
                    break;
                }

                default:
                    Serial1.print("UNKNOWN COMMAND: 0x");
                    Serial1.println(command, HEX);
                    break;
            }
        }
    }

    oled.updateDisplay(displayText.c_str(), downlink_payload ? downlink_payload : "ok");
    displayText = "";
    delay(100);
}




// #include <Arduino.h>
// #define BUTTON_PIN PC13

// void setup() {
//     pinMode(BUTTON_PIN, INPUT);
//     Serial1.begin(115200);
// }

// void loop() {
//     int state = digitalRead(BUTTON_PIN);

//     if(state == LOW) {
//         Serial1.println("Button Pressed");
//     } else {
//         Serial1.println("Button Released");
//     }

//     delay(100);
// }
