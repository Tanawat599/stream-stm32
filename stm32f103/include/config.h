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

// ===== ANALOG CONFIG =====
#define ANALOG_PIN  PA4


// ===== SD Card =====
#define SD_MOSI PB15
#define SD_MISO PB14
#define SD_SCK  PB13
#define SD_CS   PA8

// ===== LS Switch =====
#define LS_SW_PIN PB5
#define LS_SW_PORT  GPIOB


// ===== Button CONFIG =====
#define BUTTON_PIN PC13

// ===== Battery ADC CONFIG =====
#define VBATT_PIN      PA1
#define CURRENT_PIN    PA0

#define CHRG_PIN       PB0
#define DONE_PIN       PB1

const float ADC_VREF = 3.3f;
const int ADC_RES = 4095;


const float VBATT_DIVIDER = 0.75f;

const float SHUNT_RESISTOR = 150.0f;

/* ===================== Serial Configuration ===================== */
#define SERIAL_BAUD 115200


/* ===================== LORaWan Keys ===================== */
// LoRaWAN keys (MSB)
extern uint64_t joinEUI;
extern uint64_t devEUI;
extern uint8_t appKey[16];
extern uint8_t nwkKey[16];





#endif
