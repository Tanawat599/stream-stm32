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
    pinMode(LED_PIN, OUTPUT);

    if (currentActivation == MODE_OTAA) {
        Serial1.println(F("Joining via OTAA..."));
        
        state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
        state = node.activateOTAA();
        
        while (state != RADIOLIB_LORAWAN_NEW_SESSION && state != RADIOLIB_ERR_NONE) {
            Serial1.print(F("Join fail (OTAA), code: "));
            Serial1.println(state);
            Serial1.println(F("Retrying in 20 seconds..."));
            
            delay(20000); 
            
            Serial1.println(F("Re-Joining via OTAA..."));
            node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
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
void LoRaWan::loadConfig(const JsonObject& lora) {
    Serial1.println(F("\n[LoRa] Loading Config..."));

    if (lora.isNull()) {
        Serial1.println(F("[LoRa] ERROR: Config is NULL"));
        return;
    }

    JsonObject lorawan = lora["lorawan"];
    if (lorawan.isNull()) {
        Serial1.println(F("[LoRa] No 'lorawan' section"));
        return;
    }

    // ===== MODE =====
    const char* modeStr = lorawan["mode"] | "OTAA";
    if (strcmp(modeStr, "ABP") == 0) {
        currentActivation = MODE_ABP;
    } else {
        currentActivation = MODE_OTAA;
    }
    Serial1.printf("[LoRa] Mode: %s\n", modeStr);

    // ===== FPORT =====
    currentFPort = lorawan["fport"] | 2;

    // ===== CONFIRMED =====
    currentAck = lorawan["confirmed_uplink"] | false;

    // ===== TX =====
    JsonObject tx = lorawan["tx"];
    if (!tx.isNull()) {
        currentADR = tx["adr"] | true;
        currentSF  = tx["sf"]  | 7;

        node.setADR(currentADR);
        node.setDatarate(12 - currentSF);

        if (tx.containsKey("power")) {
            node.setTxPower(tx["power"]);
        }
    }

    // ===== UPLINK INTERVAL =====
    uplinkIntervalMs = (lorawan["uplink_interval_sec"] | 60) * 1000UL;

    // ===== OTAA =====
    JsonObject otaa = lorawan["otaa"];
    if (!otaa.isNull()) {
        joinEUI = parseHexToUint64(otaa["join_eui"]);
        devEUI  = parseHexToUint64(otaa["dev_eui"]);
        parseHexToBytes(otaa["app_key"], appKey, 16);
    }

    // ===== ABP =====
    JsonObject abp = lorawan["abp"];
    if (!abp.isNull()) {
        devAddr = parseHexToUint32(abp["dev_addr"]);
        parseHexToBytes(abp["nwk_skey"], nwkSEncKey, 16);
        parseHexToBytes(abp["app_skey"], appSKey, 16);
    }

    // ===== CLASS =====
    JsonObject cls = lorawan["class"];
    if (!cls.isNull()) {
        const char* type = cls["class_type"] | "A";
        if (strcmp(type, "C") == 0) setMode(CLASS_C);
        else setMode(CLASS_A);
    }

    // ===== DUTY =====
    node.setDutyCycle(lorawan["duty_cycle"] | true);

    // ===== RX2 =====
    JsonObject rx2 = lorawan["rx2"];
    if (!rx2.isNull()) {
        const char* drStr = rx2["data_rate"] | "DR2";

        uint8_t dr = 2;
        if      (strcmp(drStr, "DR0") == 0) dr = 0;
        else if (strcmp(drStr, "DR1") == 0) dr = 1;
        else if (strcmp(drStr, "DR2") == 0) dr = 2;
        else if (strcmp(drStr, "DR3") == 0) dr = 3;
        else if (strcmp(drStr, "DR4") == 0) dr = 4;
        else if (strcmp(drStr, "DR5") == 0) dr = 5;

        node.setRx2Dr(dr);
    }

    Serial1.println(F("[LoRa] Config Loaded OK\n"));
}

void LoRaWan::loadConfigFromStruct(const LoRaCfg& cfg) {

    Serial1.println(F("\n========== LORAWAN CONFIG =========="));

    // ================= MODE =================
    Serial1.print(F("Mode                : "));
    Serial1.println(cfg.lorawan.mode);

    const char* modeStr = cfg.lorawan.mode;

    if (strcmp(modeStr, "ABP") == 0) {
        currentActivation = MODE_ABP;
    }
    else {
        currentActivation = MODE_OTAA;
    }

    // ================= FPORT =================
    currentFPort = cfg.lorawan.fport;

    Serial1.print(F("FPort               : "));
    Serial1.println(currentFPort);

    // ================= CONFIRMED =================
    currentAck = cfg.lorawan.confirmed_uplink;

    Serial1.print(F("Confirmed Uplink    : "));
    Serial1.println(currentAck);

    // ================= ADR =================
    currentADR = cfg.lorawan.tx_adr;

    Serial1.print(F("ADR                 : "));
    Serial1.println(currentADR);

    // ================= SF =================
    currentSF = cfg.lorawan.tx_sf;

    Serial1.print(F("Spreading Factor    : SF"));
    Serial1.println(currentSF);

    node.setADR(currentADR);

    node.setDatarate(12 - currentSF);

    // ================= UPLINK INTERVAL =================
    uplinkIntervalMs = (cfg.lorawan.uplink_interval_sec) * 1000UL;

    Serial1.print(F("Uplink Interval     : "));
    Serial1.print(cfg.lorawan.uplink_interval_sec);
    Serial1.println(F(" sec"));

    // ================= DUTY CYCLE =================
    Serial1.print(F("Duty Cycle          : "));
    Serial1.println(cfg.lorawan.duty_cycle);

    node.setDutyCycle(cfg.lorawan.duty_cycle);

    // =====================================================
    // ======================= OTAA ========================
    // =====================================================

    Serial1.println(F("\n---------- OTAA ----------"));

    Serial1.print(F("Join EUI            : "));
    Serial1.println(cfg.lorawan.otaa.join_eui);

    Serial1.print(F("Dev EUI             : "));
    Serial1.println(cfg.lorawan.otaa.dev_eui);

    Serial1.print(F("App Key             : "));
    Serial1.println(cfg.lorawan.otaa.app_key);

    if (strlen(cfg.lorawan.otaa.join_eui) > 0) {

        joinEUI = parseHexToUint64(cfg.lorawan.otaa.join_eui);

        devEUI  = parseHexToUint64(cfg.lorawan.otaa.dev_eui);

        parseHexToBytes(cfg.lorawan.otaa.app_key, appKey, 16);
    }

    // =====================================================
    // ======================== ABP ========================
    // =====================================================

    Serial1.println(F("\n---------- ABP ----------"));

    Serial1.print(F("Dev Addr            : "));
    Serial1.println(cfg.lorawan.abp.dev_addr);

    Serial1.print(F("NwkSKey             : "));
    Serial1.println(cfg.lorawan.abp.nwk_skey);

    Serial1.print(F("AppSKey             : "));
    Serial1.println(cfg.lorawan.abp.app_skey);

    if (strlen(cfg.lorawan.abp.dev_addr) > 0) {

        devAddr = parseHexToUint32(cfg.lorawan.abp.dev_addr);

        parseHexToBytes(cfg.lorawan.abp.nwk_skey, nwkSEncKey, 16);

        parseHexToBytes(cfg.lorawan.abp.app_skey, appSKey, 16);
    }

    // =====================================================
    // ======================== CLASS ======================
    // =====================================================

    Serial1.print(F("\nClass               : "));
    Serial1.println(cfg.lorawan.class_type);

    if (strcmp(cfg.lorawan.class_type, "C") == 0) {
        setMode(CLASS_C);
    }
    else {
        setMode(CLASS_A);
    }

    Serial1.println(F("\n[LoRa] Config Loaded Successfully"));
    Serial1.println(F("====================================\n"));
}

// ===== LOOP =====
void LoRaWan::loop(const char* payload) {

    // ================= UPLINK =================
    if (millis() - lastSend >= uplinkIntervalMs) {
        lastSend = millis();

        Serial1.println(F("\n===== UPLINK ====="));
        Serial1.print(F("Payload: "));
        Serial1.println(payload);

        uint8_t rxBuffer[255];
        size_t rxLen = 0;

        int16_t state = node.sendReceive(
            (uint8_t*)payload,
            strlen(payload),
            currentFPort,
            rxBuffer,  
            &rxLen,    
            currentAck
        );

        if (state >= RADIOLIB_ERR_NONE) {
            Serial1.print(F("[UPLINK] Success! "));
            
            if (state == 0) {
                Serial1.println(F("(No Downlink)"));
            } 
            else if (state == 1 || state == 2) {
                Serial1.print(F("(Downlink in RX")); Serial1.print(state); Serial1.println(F(")"));
                
                if (rxLen > 0) {
                    Serial1.println(F("\n===== DOWNLINK RECEIVED (CLASS A) ====="));
                    Serial1.print(F("RAW HEX: "));
                    
                    size_t idx = 0;
                    for (size_t i = 0; i < rxLen; i++) {
                        // ปริ้นต์ลง Serial Monitor
                        if (rxBuffer[i] < 0x10) Serial1.print('0');
                        Serial1.print(rxBuffer[i], HEX); Serial1.print(" ");

                        if (idx < sizeof(downlinkText) - 2) {
                            snprintf(&downlinkText[idx], 3, "%02X", rxBuffer[i]);
                            idx += 2;
                        }
                    }
                    Serial1.println();

                    downlinkText[idx] = '\0'; 
                    hasNewDownlink = true;    
                    
                    Serial1.print(F("STORED PAYLOAD: "));
                    Serial1.println(downlinkText);
                    Serial1.println(F("======================================="));
                } else {
                    Serial1.println(F("-> (Network MAC Command / No Payload)"));
                }
            }
        } 
        else if (state == RADIOLIB_ERR_ACK_NOT_RECEIVED) {
            Serial1.println(F("[UPLINK] NO ACK"));
        }
        else {
            Serial1.print(F("[UPLINK] ERROR CODE: "));
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
            Serial1.print(F("RAW HEX: "));
            
            size_t idx = 0;
            for (size_t i = 0; i < len; i++) {
                // ปริ้นต์ลง Serial Monitor
                if (buf[i] < 0x10) Serial1.print('0');
                Serial1.print(buf[i], HEX); Serial1.print(" ");

                if (idx < sizeof(downlinkText) - 2) {
                    snprintf(&downlinkText[idx], 3, "%02X", buf[i]);
                    idx += 2;
                }
            }
            Serial1.println();

            downlinkText[idx] = '\0';  
            hasNewDownlink = true;     
            
            Serial1.print(F("STORED PAYLOAD: "));
            Serial1.println(downlinkText);
            Serial1.println(F("============================"));
        }
    }
} 


bool LoRaWan::available() {
    return hasNewDownlink; 
}

const char* LoRaWan::getDownlink() {
    hasNewDownlink = false;
    return downlinkText;
}



// ===== END =====
void LoRaWan::end() {
    SPI.end();
    digitalWrite(LORA_SS_PIN, HIGH);
    Serial1.println(F("LoRa SPI Closed"));
}