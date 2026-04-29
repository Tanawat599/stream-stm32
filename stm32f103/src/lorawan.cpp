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
uint64_t joinEUI = 0xE63BA610B2498DBE;
uint64_t devEUI  = 0xC304DB83070E0063;
uint8_t appKey[16] = { 0x28, 0xA7, 0x7C, 0xA4, 0x83, 0x7A, 0x95, 0x1F, 0x42, 0xA2, 0xD7, 0xA3, 0x14, 0x93, 0x04, 0x3C};
uint8_t nwkKey[16] = { 0x28, 0xA7, 0x7C, 0xA4, 0x83, 0x7A, 0x95, 0x1F, 0x42, 0xA2, 0xD7, 0xA3, 0x14, 0x93, 0x04, 0x3C};

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
        
        state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
        state = node.activateOTAA();
        
        while (state != RADIOLIB_LORAWAN_NEW_SESSION && state != -1108) {
            Serial1.print(F("Join fail (OTAA), code: "));
            Serial1.println(state);
            Serial1.println(F("Retrying in 20 seconds..."));
            
            delay(20000); 
            
            Serial1.println(F("Re-Joining via OTAA..."));
            state = node.activateOTAA(); 
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

    lastSend = millis() - uplinkIntervalMs;
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
        if (lorawan.containsKey("uplink_interval_sec")) {

            uplinkIntervalMs = lorawan["uplink_interval_sec"].as<unsigned long>() * 1000UL;
            Serial1.print("uplinkIntervalMs :"); Serial1.println(uplinkIntervalMs);
        
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
    if (lorawan.containsKey("rx2")) {
        JsonObject rx2 = lorawan["rx2"].as<JsonObject>();
        Serial1.println(F("LoRa: RX2 Config found"));
        
        if (rx2.containsKey("data_rate")) {
            const char* rx2DR_str = rx2["data_rate"];
            uint8_t dr_val = 2;
            
            if (strcmp(rx2DR_str, "DR0") == 0) dr_val = 0;
            else if (strcmp(rx2DR_str, "DR1") == 0) dr_val = 1;
            else if (strcmp(rx2DR_str, "DR2") == 0) dr_val = 2;
            else if (strcmp(rx2DR_str, "DR3") == 0) dr_val = 3;
            else if (strcmp(rx2DR_str, "DR4") == 0) dr_val = 4;
            else if (strcmp(rx2DR_str, "DR5") == 0) dr_val = 5;

            node.setRx2Dr(dr_val);
            
            Serial1.print(F("LoRa: RX2 Data Rate applied as DR"));
            Serial1.println(dr_val);
        }
    }


}

// ===== LOOP =====
void LoRaWan::loop(const char* payload) {

    // ================= UPLINK (สำหรับทั้ง Class A และ C) =================
    if (millis() - lastSend >= uplinkIntervalMs) {
        lastSend = millis();

        Serial1.println(F("\n===== UPLINK ====="));
        Serial1.print(F("Payload: "));
        Serial1.println(payload);
        Serial1.print(F("FPort: ")); Serial1.println(currentFPort);
        Serial1.print(uplinkIntervalMs); Serial1.println(F(" ms interval"));

        uint8_t rxBuffer[255];
        size_t rxLen = 0;

        int16_t state = node.sendReceive(
            (uint8_t*)payload,
            strlen(payload),
            currentFPort,
            currentAck
        );

        if (state >= RADIOLIB_ERR_NONE) {
            Serial1.print(F("[UPLINK] Success! "));
            if (state == 0) {
                Serial1.println(F("(No Downlink)"));
            } 
            else if (state == 1 || state == 2) {
                Serial1.print(F("(Downlink in RX")); Serial1.print(state); Serial1.println(F(")"));
                

            }
            else if (state == 3) {
                Serial1.println(F("(Downlink in RXC)"));
            }
        } 
        else if (state == RADIOLIB_ERR_ACK_NOT_RECEIVED) {
            Serial1.println(F("[UPLINK] NO ACK (Confirmed Uplink Failed)"));
        }
        else {
            Serial1.print(F("[UPLINK] ACTUAL ERROR: "));
            Serial1.println(state);
        }
    }

    // ================= CLASS C DOWNLINK =================
    if (currentMode == CLASS_C) {
        uint8_t buf[255];
        size_t len = 0;
        int16_t dl = node.getDownlinkClassC(buf, &len, NULL);

        if (dl > 0 && len > 0) {
            Serial1.println(F("\n===== DOWNLINK RECEIVED (CLASS C) ====="));
            Serial1.print(F("From Window: ")); Serial1.println(dl);
            
            Serial1.print(F("HEX: "));
            for (size_t i = 0; i < len; i++) {
                if (buf[i] < 0x10) Serial1.print('0');
                Serial1.print(buf[i], HEX); Serial1.print(" ");
            }
            Serial1.println();

            Serial1.print(F("TEXT: "));
            for (size_t i = 0; i < len; i++) {
                if (isprint(buf[i])) Serial1.print((char)buf[i]);
                else Serial1.print('.');
            }
            Serial1.println();
            Serial1.println(F("============================"));
        }
    }
}

// ===== END =====
void LoRaWan::end() {
    SPI.end();
    digitalWrite(LORA_SS_PIN, HIGH);
    Serial1.println(F("LoRa SPI Closed"));
}