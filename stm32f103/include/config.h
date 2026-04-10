/* =========================================================
 *                      PROJECT CONFIG
 * =========================================================
 * File: config.h
 * Description: Central configuration for hardware + features
 * =========================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

/* ===================== Configuration ===================== */
// ===== LoRa CONFIG =====
#define LORA_SS_PIN PB11
#define LORA_RST_PIN PB12
#define LORA_DIO0_PIN PB0
#define LORA_DIO1_PIN PB1

// ===== LED CONFIG =====
#define LED_PIN PA12

// ===== OLED CONFIG =====
#define OLED_SDA_PIN PB7
#define OLED_SCL_PIN PB6

#define OLED_SDA     PB7
#define OLED_SCL     PB6

#define OLED_ADDR    0x3C

// ===== RS485 CONFIG =====
#define RS485_SERIAL Serial2
#define RS485_DE_PIN PB9
#define RS485_RE_PIN PB8
#define RS485_BAUD 9600


#define SLAVE_ADDR_1 0x08
#define SLAVE_ADDR_2 0x09

// ===== RS485 CONFIG =====
#define SD_MOSI PB15
#define SD_MISO PB14
#define SD_SCK  PB13
#define SD_CS   PA8

/* ===================== Serial Configuration ===================== */
#define SERIAL_BAUD 115200


/* ===================== LORaWan Keys ===================== */
// LoRaWAN keys (MSB)
const uint64_t joinEUI = 0xFC644250F0DB9BE9;
const uint64_t devEUI  = 0x9F75EDA1CF67BF63;
const uint8_t appKey[] = { 0xB2, 0x29, 0x49, 0x8B, 0xBF, 0xC7, 0xD8, 0xE6, 0xD2, 0xDB, 0x81, 0x04, 0xD3, 0x8A, 0x4D, 0x24 };
const uint8_t nwkKey[] = { 0xB2, 0x29, 0x49, 0x8B, 0xBF, 0xC7, 0xD8, 0xE6, 0xD2, 0xDB, 0x81, 0x04, 0xD3, 0x8A, 0x4D, 0x24 };





#endif
