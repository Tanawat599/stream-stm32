// #include <Arduino.h>
// #include <RadioLib.h>
// #include "config.h"

// // Pin setup
// Module module(LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_DIO1_PIN);
// SX1276 radio(&module);
// LoRaWANNode node(&radio, &AS923);

// unsigned long lastSend = 0;

// void setup() {
//     Serial1.begin(115200);
//     delay(2000);

//     Serial1.println(F("Initializing radio..."));
//     int16_t state = radio.begin();
//     if (state != RADIOLIB_ERR_NONE) {
//         Serial1.print(F("Radio init failed: "));
//         Serial1.println(state);
//         while (true);
//     }
//     Serial1.println(F("Radio OK"));

//     // OTAA init
//     state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
//     if (state != RADIOLIB_ERR_NONE) {
//         Serial1.print(F("OTAA init failed: "));
//         Serial1.println(state);
//         while (true);
//     }
//     Serial1.println(F("Joined network!"));
// }

// void loop() {
//     // ===== CLASS A (STM32 only send uplink) =====
//     if (millis() - lastSend > 10000) {
//         lastSend = millis();

//         const char* msg = "Hello STM32 Class A";

//         int16_t state = node.sendUplink((uint8_t*)msg, strlen(msg), 1, true);

//         if (state == RADIOLIB_ERR_NONE) {
//             Serial1.println(F("Uplink sent OK"));
//         } else {
//             Serial1.print(F("Send uplink error: "));
//             Serial1.println(state);
//         }
//     }

//     // ===== CLASS C (รับ downlink) =====
//     uint8_t downlink[255];
//     size_t len = 0;
//     LoRaWANEvent_t event;

//     int16_t state = node.getDownlinkClassC(downlink, &len, &event);

//     if (state > 0 && len > 0) {
//         Serial1.print(F("Downlink: "));
//         for (size_t i = 0; i < len; i++) {
//             Serial1.print((char)downlink[i]);
//         }
//         Serial1.println();
//     }
// }

#include <RadioLib.h>
#include <Arduino.h>
#include "config.h"
#include "mylib.h"


Module module(LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_DIO1_PIN);
SX1276 radio(&module);
LoRaWANNode node(&radio, &AS923);

void LoRaWan::begin(){
    Serial1.println(F("\nSetup LoRaWAN Class C..."));
    Serial1.print(F("Initialise radio... "));

    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial1.println(F("failed!"));
        while (true);
    }
    Serial1.println(F("success!"));

    Serial1.print(F("Join Network... "));
    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);

    state = node.activateOTAA();
    if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
        Serial1.print(F("failed, code: ")); Serial1.println(state);
        while (true);
    }
    Serial1.println(F("Joined!"));

    node.setClass(RADIOLIB_LORAWAN_CLASS_C);

    const char* payload = "Hello";
    Serial1.println(F("Sending first uplink..."));
    state = node.sendReceive((uint8_t*)payload, strlen(payload), 1, true);
}

// void LoRaWan::classC(){
//   uint8_t downlinkPayload[255];
//   size_t downlinkLen = 0;
//   LoRaWANEvent_t downlinkEvent;

//   int16_t state = node.getDownlinkClassC(downlinkPayload, &downlinkLen, &downlinkEvent);
  
//   if(state > 0 && downlinkLen > 0) {
//     Serial1.print(F("Received (HEX): "));
//     for (size_t i = 0; i < downlinkLen; i++) {
//       if (downlinkPayload[i] < 0x10) Serial1.print('0');
//       Serial1.print(downlinkPayload[i], HEX);
//       Serial1.print(" ");
//     }
//     Serial1.println();

//     Serial1.print(F("Received (Text): "));
//     for (size_t i = 0; i < downlinkLen; i++) {
//       if (isprint(downlinkPayload[i])) { 
//         Serial1.print((char)downlinkPayload[i]);
//       } else {
//         Serial1.print('.');
//       }
//     }
//     Serial1.println();
//     Serial1.println(F("-------------------"));
//   } 
//   else {
//     Serial1.print(F("Error receiving downlink, code: ")); Serial1.println(state);
//     Serial1.println(F("-------------------"));
//   }
// }

void LoRaWan::classC() {
    static unsigned long lastCheck = 0;

    // ตรวจสอบ downlink ทุก 1000ms (1 วินาที)
    if (millis() - lastCheck < 1000) return;
    lastCheck = millis();

    uint8_t downlinkPayload[255];
    size_t downlinkLen = 0;
    LoRaWANEvent_t downlinkEvent;

    int16_t state = node.getDownlinkClassC(downlinkPayload, &downlinkLen, &downlinkEvent);

    if(state > 0 && downlinkLen > 0) {
        Serial1.print(F("Received (HEX): "));
        for (size_t i = 0; i < downlinkLen; i++) {
            if (downlinkPayload[i] < 0x10) Serial1.print('0');
            Serial1.print(downlinkPayload[i], HEX);
            Serial1.print(" ");
        }
        Serial1.println();

        Serial1.print(F("Received (Text): "));
        for (size_t i = 0; i < downlinkLen; i++) {
            if (isprint(downlinkPayload[i])) Serial1.print((char)downlinkPayload[i]);
            else Serial1.print('.');
        }
        Serial1.println();
        Serial1.println(F("-------------------"));
    } else {
        // ไม่ต้องพิมพ์รัว ถ้าไม่มี downlink
        // Serial1.print(F("Error receiving downlink, code: ")); 
        // Serial1.println(state);
    }
}