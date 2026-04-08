#include <RadioLib.h>
#include <Arduino.h>
#include "config.h"
#include "mylib.h"
// ===== MODE =====


LoRaClassMode currentMode = CLASS_A;

// ===== LORA =====
Module module(LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_DIO1_PIN);
SX1276 radio(&module);
LoRaWANNode node(&radio, &AS923);

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
    Serial1.begin(115200);
    delay(2000);

    Serial1.println(F("\nLoRaWAN Hybrid"));

    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial1.println(F("Radio fail"));
        while (true);
    }

    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    state = node.activateOTAA();

    if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
        Serial1.println(F("Join fail"));
        while (true);
    }

    Serial1.println(F("Joined"));

    setMode(currentMode);
}

// ===== LOOP =====
void LoRaWan::loop(const char* payload) {

    // ===== SEND =====
    if (millis() - lastSend > 10000) {
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
                Serial1.print(F("[A] Error: "));
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
                Serial1.print(F("[C] Error: "));
                Serial1.println(state);
            }
        }

        Serial1.println(F("-------------------"));
    }

    // ===== CLASS C RECEIVE =====
    if (currentMode == CLASS_C) {

        if (millis() - lastCheck > 1000) {
            lastCheck = millis();

            uint8_t payload[255];
            size_t len = 0;
            LoRaWANEvent_t event;

            int16_t state = node.getDownlinkClassC(payload, &len, &event);

            if (state > 0 && len > 0) {
                Serial1.print(F("[C] Downlink: "));
                for (size_t i = 0; i < len; i++) {
                    Serial1.print((char)payload[i]);
                }
                Serial1.println();
            }
        }
    }
}