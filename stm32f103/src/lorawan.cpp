#include <RadioLib.h>
#include <Arduino.h>
#include "config.h"
#include "mylib.h"
// ===== MODE =====


LoRaClassMode currentMode = CLASS_C;

// ===== LORA =====
Module module(LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_DIO1_PIN);
SX1276 radio(&module);
LoRaWANNode node(&radio, &AS923);

// ===== Runtime-configurable LoRaWAN keys (defaults kept here)
uint64_t joinEUI = 0xFC644250F0DB9BE9;
uint64_t devEUI  = 0x9F75EDA1CF67BF63;
uint8_t appKey[16] = { 0xB2, 0x29, 0x49, 0x8B, 0xBF, 0xC7, 0xD8, 0xE6, 0xD2, 0xDB, 0x81, 0x04, 0xD3, 0x8A, 0x4D, 0x24};
uint8_t nwkKey[16] = { 0xB2, 0x29, 0x49, 0x8B, 0xBF, 0xC7, 0xD8, 0xE6, 0xD2, 0xDB, 0x81, 0x04, 0xD3, 0x8A, 0x4D, 0x24 };

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
    // If shorter, parse as available
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

    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    state = node.activateOTAA();
    Serial1.println(joinEUI);
    if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
        Serial1.println(F("Join fail"));
        while (true);
    }

    Serial1.println(F("Joined"));

    setMode(currentMode);
    node.setADR(false);
}

void LoRaWan::loadConfig(SDResourceManager& sd, const char* path) {
    String json = sd.readFile(path);
    if (json == "ERROR_OPEN") {
        Serial1.print(F("LoRa: failed to open config " ));
        Serial1.println(path);
        return;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial1.println(F("LoRa: JSON parse failed"));
        return;
    }

    JsonObject lora = doc["lora"].as<JsonObject>();
    if (!lora.containsKey("lorawan")) return;
    JsonObject lorawan = lora["lorawan"].as<JsonObject>();
    if (!lorawan.containsKey("otaa")) return;
    JsonObject otaa = lorawan["otaa"].as<JsonObject>();

    if (otaa.containsKey("join_eui")) {
        const char* s = otaa["join_eui"];
        joinEUI = parseHexToUint64(s);
        Serial1.print(F("LoRa: joinEUI set: 0x")); printUint64Hex(joinEUI);
    }

    if (otaa.containsKey("dev_eui")) {
        const char* s = otaa["dev_eui"];
        devEUI = parseHexToUint64(s);
        Serial1.print(F("LoRa: devEUI set: 0x")); printUint64Hex(devEUI);
    }

    if (otaa.containsKey("nwk_key")) {
        const char* s = otaa["nwk_key"];
        parseHexToBytes(s, nwkKey, 16);
        Serial1.println(F("LoRa: nwkKey set"));
    }

    if (otaa.containsKey("app_key")) {
        const char* s = otaa["app_key"];
        parseHexToBytes(s, appKey, 16);
        Serial1.println(F("LoRa: appKey set"));
    }

    if (lorawan.containsKey("class")) {
        const char* s = lorawan["class"];
        if (strcmp(s, "A") == 0) {
            setMode(CLASS_A);
        } else if (strcmp(s, "C") == 0) {
            setMode(CLASS_C);
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

    if (lorawan.containsKey("rx2")) {
        JsonObject rx2 = lorawan["rx2"].as<JsonObject>();
        if (rx2.containsKey("frequency")) {
            float freq = rx2["frequency"];
            //node.setRx2Frequency(freq);
            Serial1.print(F("LoRa: RX2 Freq set: ")); Serial1.println(freq);
        }
        if (rx2.containsKey("data_rate")) {
            int sf = rx2["data_rate"];
            //node.setRx2SpreadingFactor(sf);
            Serial1.print(F("LoRa: RX2 SF set: ")); Serial1.println(sf);
        }
    }

    if (lorawan.containsKey("confirmed_uplink")){
        bool confirmed = lorawan["confirmed_uplink"];
        //node.setConfirmedUplink(confirmed);
        Serial1.print(F("LoRa: Confirmed Uplink set: ")); Serial1.println(confirmed ? "ON" : "OFF");
    }

    if (lorawan.containsKey("fport")) {
        int fport = lorawan["fport"];
        //node.setFPort(fport);
        Serial1.print(F("LoRa: FPort set: ")); Serial1.println(fport);

    }

    if (lorawan.containsKey("duty_cycle")) {
        bool duty = lorawan["duty_cycle"];
        node.setDutyCycle(duty);
        Serial1.print(F("LoRa: Duty Cycle set: ")); Serial1.println(duty ? "ON" : "OFF");
    }


}

// ===== LOOP =====
void LoRaWan::loop(const char* payload) {

    // ===== SEND =====
    if (millis() - lastSend > 20000) {
        lastSend = millis();

        // const char* payload = "Hello Hybrid";
        Serial1.print(F("Sending: "));
        Serial1.println(payload);

        if (currentMode == CLASS_A) {

            int16_t state = node.sendReceive(
                (uint8_t*)payload,
                strlen(payload),
                1,
                true
            );

            if (state == RADIOLIB_ERR_NONE) {
                Serial1.println(F("[A] Uplink OK"));
            } else {
                Serial1.print(F("[A] State: "));
                Serial1.println(state);
            }

        } else {
            int16_t state = node.sendReceive(
                (uint8_t*)payload,
                strlen(payload),
                1,
                true
            );

            if (state == RADIOLIB_ERR_NONE) {
                Serial1.println(F("[C] Uplink OK"));
            } else {
                Serial1.print(F("[C] State: "));
                Serial1.println(state);
            }
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
void LoRaWan::end() {
    SPI.end();
    digitalWrite(LORA_SS_PIN, HIGH);       
    Serial1.println(F("LoRa SPI Closed"));
}