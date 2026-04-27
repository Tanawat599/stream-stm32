#include <RadioLib.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include "config.h"
#include "mylib.h"

// ===== MODE & ACTIVATION =====
enum ActivationMode { MODE_OTAA, MODE_ABP };

LoRaClassMode currentMode = CLASS_C;
ActivationMode currentActivation = MODE_OTAA; 

uint8_t currentFPort = 2;      
bool currentAck = false;
unsigned long uplinkIntervalMs = 20000; 
bool currentADR = true;    
uint8_t currentSF = 7;     
// ===== LORA =====
Module module(LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_DIO1_PIN);
SX1276 radio(&module);
LoRaWANNode node(&radio, &AS923);

// ===== OTAA Keys (Defaults) =====
uint64_t joinEUI = 0xFC644250F0DB9BE9;
uint64_t devEUI  = 0x9F75EDA1CF67BF63;
uint8_t appKey[16] = { 0xB2, 0x29, 0x49, 0x8B, 0xBF, 0xC7, 0xD8, 0xE6, 0xD2, 0xDB, 0x81, 0x04, 0xD3, 0x8A, 0x4D, 0x24};
uint8_t nwkKey[16] = { 0xB2, 0x29, 0x49, 0x8B, 0xBF, 0xC7, 0xD8, 0xE6, 0xD2, 0xDB, 0x81, 0x04, 0xD3, 0x8A, 0x4D, 0x24};

// ===== ABP Keys (Defaults) =====
uint32_t devAddr = 0x00000000;
uint8_t nwkSEncKey[16]  = {0};
uint8_t appSKey[16]     = {0};

// ===== Helpers: parse hex strings from JSON =====
static uint8_t hexCharToNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return 0;
}

static void parseHexToBytes(const char* hex, uint8_t* out, size_t outLen) {
    for (size_t i = 0; i < outLen; i++) {
        uint8_t hi = hexCharToNibble(hex[i*2]);
        uint8_t lo = hexCharToNibble(hex[i*2 + 1]);
        out[i] = (hi << 4) | lo;
    }
}

static uint64_t parseHexToUint64(const char* hex) {
    uint64_t v = 0;
    size_t len = strlen(hex);
    for (size_t i = 0; i < len; i++) {
        char c = hex[i];
        uint8_t nib = hexCharToNibble(c);
        v = (v << 4) | nib;
    }
    return v;
}

static uint32_t parseHexToUint32(const char* hex) {
    uint32_t v = 0;
    size_t len = strlen(hex);
    for (size_t i = 0; i < len; i++) {
        char c = hex[i];
        uint8_t nib = hexCharToNibble(c);
        v = (v << 4) | nib;
    }
    return v;
}

static void printUint64Hex(uint64_t v) {
    char buf[17];
    for (int i = 0; i < 8; i++) {
        uint8_t byte = (v >> ((7 - i) * 8)) & 0xFF;
        sprintf(buf + i*2, "%02X", byte);
    }
    buf[16] = '\0';
    Serial1.println(buf);
}

unsigned long lastSend = 0;
unsigned long lastCheck = 0;

// ===== SET MODE =====
void LoRaWan::setMode(LoRaClassMode mode) {
    currentMode = mode;

    if (mode == CLASS_A) {
        node.setClass(RADIOLIB_LORAWAN_CLASS_A);
        Serial1.println(F("Mode: Class A"));
    } else {
        node.setClass(RADIOLIB_LORAWAN_CLASS_C);
        Serial1.println(F("Mode: Class C"));
    }
}

// ===== SETUP =====
void LoRaWan::begin() {
    Serial1.println(F("\nLoRaWAN Hybrid"));

    SPI.setSCLK(PA5);
    SPI.setMISO(PA6);
    SPI.setMOSI(PA7);
    SPI.begin();

    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial1.println(F("Radio fail"));
        Serial1.println(state);
        while (true);
    }

    if (currentActivation == MODE_OTAA) {
        Serial1.println(F("Joining via OTAA..."));
        state = node.beginOTAA(joinEUI, devEUI, NULL, appKey);
        state = node.activateOTAA();
        if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
            Serial1.print(F("Join fail (OTAA), code: "));
            Serial1.println(state);
            while (true);
        }
        Serial1.println(F("Joined OTAA successfully"));
    } else {
        Serial1.println(F("Activating via ABP..."));
        state = node.beginABP(devAddr, NULL, NULL, nwkSEncKey, appSKey);
        state = node.activateABP();
        if (state != RADIOLIB_ERR_NONE && state != RADIOLIB_LORAWAN_NEW_SESSION) {
            Serial1.print(F("Activate fail (ABP), code: "));
            Serial1.println(state);
            while (true);
        }
        Serial1.println(F("Activated ABP successfully"));
    }
    if (state == RADIOLIB_LORAWAN_NEW_SESSION || state == RADIOLIB_ERR_NONE) {
        Serial1.println(F("Activation Success! Applying Config..."));

        node.setADR(currentADR); 


        uint8_t dr = 12 - currentSF; 
        if (dr > 5) dr = 5; 
        node.setDatarate(dr);

        Serial1.print(F("LoRa: ADR is ")); Serial1.println(currentADR ? F("ON") : F("OFF"));
        Serial1.print(F("LoRa: SF forced to ")); Serial1.println(currentSF);
    }
    setMode(currentMode);

}

// ===== CONFIG =====
void LoRaWan::loadConfig(SDResourceManager& sd, const char* path) {
    String json = sd.readFile(path);
    if (json == "ERROR_OPEN") {
        Serial1.print(F("LoRa: failed to open config "));
        Serial1.println(path);
        return;
    }

    StaticJsonDocument<2048> doc; 
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial1.print(F("LoRa: JSON parse failed: "));
        Serial1.println(err.c_str());
        return;
    }

    JsonObject lora = doc["lora"].as<JsonObject>();
    if (!lora.containsKey("lorawan")) return;
    JsonObject lorawan = lora["lorawan"].as<JsonObject>();

    if (lorawan.containsKey("mode")) {
        const char* modeStr = lorawan["mode"];
        if (strcmp(modeStr, "ABP") == 0) {
            currentActivation = MODE_ABP;
            Serial1.println(F("LoRa: Mode set to ABP"));
        } else {
            currentActivation = MODE_OTAA;
            Serial1.println(F("LoRa: Mode set to OTAA"));
        }
    }
    if (lorawan.containsKey("fport")) {
        currentFPort = lorawan["fport"];
        Serial1.print(F("LoRa: FPort set: ")); Serial1.println(currentFPort);
    }

    if (lorawan.containsKey("tx")) {
        JsonObject tx = lorawan["tx"].as<JsonObject>();
        if (tx.containsKey("adr")) {
            currentADR = tx["adr"]; 
        }
        if (tx.containsKey("sf")) {
            currentSF = tx["sf"];   
        }
    }

    if (lorawan.containsKey("confirmed_uplink")) {
        currentAck = lorawan["confirmed_uplink"];
        Serial1.print(F("LoRa: Confirmed Uplink set: ")); Serial1.println(currentAck ? "ON" : "OFF");
    }

    if (lorawan.containsKey("otaa")) {
        JsonObject otaa = lorawan["otaa"].as<JsonObject>();
        if (otaa.containsKey("join_eui")) {
            joinEUI = parseHexToUint64(otaa["join_eui"]);
            Serial1.print(F("LoRa: joinEUI set: 0x")); printUint64Hex(joinEUI);
        }
        if (otaa.containsKey("dev_eui")) {
            devEUI = parseHexToUint64(otaa["dev_eui"]);
            Serial1.print(F("LoRa: devEUI set: 0x")); printUint64Hex(devEUI);
        }
        if (otaa.containsKey("app_key")) {
            parseHexToBytes(otaa["app_key"], appKey, 16);
            Serial1.println(F("LoRa: OTAA appKey set"));
        }
    }

    if (lorawan.containsKey("abp")) {
        JsonObject abp = lorawan["abp"].as<JsonObject>();
        if (abp.containsKey("dev_addr")) {
            devAddr = parseHexToUint32(abp["dev_addr"]);
            Serial1.print(F("LoRa: DevAddr set: 0x")); Serial1.println(devAddr, HEX);
        }
        if (abp.containsKey("nwk_skey")) {
            // อ่านค่าใส่ nwkSEncKey ตรงๆ แล้วลบ memcpy ทิ้งเลยครับ
            parseHexToBytes(abp["nwk_skey"], nwkSEncKey, 16);
            Serial1.println(F("LoRa: ABP NwkSKey set"));
        }
        if (abp.containsKey("app_skey")) {
            parseHexToBytes(abp["app_skey"], appSKey, 16);
            Serial1.println(F("LoRa: ABP AppSKey set"));
        }
    }

    if (lorawan.containsKey("class")) {
        JsonObject cls = lorawan["class"].as<JsonObject>();
        if (cls.containsKey("class_type")) {
            const char* s = cls["class_type"];
            if (strcmp(s, "A") == 0) {
                setMode(CLASS_A);
            } else if (strcmp(s, "C") == 0) {
                setMode(CLASS_C);
            }
        }
        if (cls.containsKey("class_a")) {
        JsonObject clsA = cls["class_a"];
        if (clsA.containsKey("uplink_interval_sec")) {
            uplinkIntervalMs = clsA["uplink_interval_sec"].as<unsigned long>() * 1000UL;
        }
}
    }

    if (lorawan.containsKey("tx")) {
        JsonObject tx = lorawan["tx"].as<JsonObject>();
        if (tx.containsKey("adr")) {
            bool adr = tx["adr"];
            node.setADR(adr);
            Serial1.print(F("LoRa: ADR set: ")); Serial1.println(adr ? "ON" : "OFF");
        }
        if (tx.containsKey("sf")) {
            int sf = tx["sf"];
            uint8_t dr = 12 - sf; 
            node.setDatarate(dr);
            Serial1.print(F("LoRa: SF set: ")); Serial1.println(sf);
        }
        if (tx.containsKey("power")) {
            int power = tx["power"];
            node.setTxPower(power);
            Serial1.print(F("LoRa: Tx Power set: ")); Serial1.println(power);
        }
    }

    if (lorawan.containsKey("duty_cycle")) {
        bool duty = lorawan["duty_cycle"];
        node.setDutyCycle(duty);
        Serial1.print(F("LoRa: Duty Cycle set: ")); Serial1.println(duty ? "ON" : "OFF");
    }


}

// ===== LOOP =====
void LoRaWan::loop(const char* payload) {

    if (millis() - lastSend > uplinkIntervalMs) {
        lastSend = millis();

        Serial1.print(F("Sending: "));
        Serial1.println(payload);

        int16_t state = node.sendReceive((uint8_t*)payload, strlen(payload), currentFPort, currentAck);

        if (state == RADIOLIB_LORAWAN_NEW_SESSION || state == RADIOLIB_ERR_NONE || state > 0) {
if (state > 0) {
            Serial1.println(F("[A] Uplink OK & ACK Received!"));
            
            String strDown;
            int16_t res = radio.readData(strDown); 
            
            if (res == RADIOLIB_ERR_NONE && strDown.length() > 0) {
                Serial1.print(F("--> Received Payload: "));
                
                // ตรวจสอบว่าเป็นข้อความที่ "มนุษย์อ่านออก" หรือไม่
                bool readable = true;
                for (size_t i = 0; i < strDown.length(); i++) {
                    if (!isprint(strDown[i])) { // ใช้ isprint (ตัวพิมพ์เล็กหมด)
                        readable = false;
                        break;
                    }
                }

                if (readable) {
                    Serial1.println(strDown);
                } else {
                    Serial1.print(F("(Binary/MAC Command) Hex: "));
                    for (size_t i = 0; i < strDown.length(); i++) {
                        if ((unsigned char)strDown[i] < 0x10) Serial1.print('0');
                        Serial1.print((unsigned char)strDown[i], HEX);
                        Serial1.print(" ");
                    }
                    Serial1.println();
                }
            }
        } else {
                Serial1.println(F("[A] Uplink OK (No Downlink)"));
            }
        } 
        else {
            Serial1.print(F("[A] State Error: "));
            Serial1.println(state);

        }
        Serial1.println(F("-------------------"));
    }

    // ===== CLASS C RECEIVE =====
    if (currentMode == CLASS_C) {

        uint8_t downlink[255];
        size_t len = 0;
        LoRaWANEvent_t event;

        int16_t state = node.getDownlinkClassC(downlink, &len, &event);

        if (state > 0 && len > 0) {
            Serial1.print(F("[C] Downlink: "));
            for (size_t i = 0; i < len; i++) {
                Serial1.print((char)downlink[i]);
            }
            Serial1.println();
        }
    }
}

// ===== END =====
void LoRaWan::end() {
    SPI.end();
    digitalWrite(LORA_SS_PIN, HIGH);       
    Serial1.println(F("LoRa SPI Closed"));
}