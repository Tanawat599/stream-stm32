/* =========================================================
 *                      PROJECT CONFIG
 * =========================================================
 * File: config.h
 * Description: Central configuration for hardware + features
 * Author: YourName
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

/* ===================== Serial Configuration ===================== */
#define SERIAL_BAUD 115200


/* ===================== LORaWan Keys ===================== */
// Single extern declarations for LoRaWAN keys (defined in src/config.cpp)
extern uint64_t joinEUI;
extern uint64_t devEUI;
extern uint8_t appKey[];
extern uint8_t nwkKey[];



#endif
