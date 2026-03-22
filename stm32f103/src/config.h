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

/* ===================== Pin Configuration ===================== */
//LoRa
#define LORA_SS_PIN PB11
#define LORA_RST_PIN PB12
#define LORA_DIO0_PIN PB0
#define LORA_DIO1_PIN PB1
//LED
#define LED_PIN PA12
//OLED_Display
#define OLED_SDA_PIN PB7
#define OLED_SCL_PIN PB6
//RS485
#define RS485_DE_PIN PA3
#define RS485_RE_PIN PA2

/* ===================== Serial Configuration ===================== */
#define SERIAL_BAUD 115200
#define STM32_SERIAL Serial1


/* ===================== LORaWan Configuration ===================== */
class LoRaWan {
public:
    void begin();
    void classC();
    void classA();
};

//Class C
uint64_t joinEUI = 0xFE018BB657CBC193; 
uint64_t devEUI  = 0x9246b8a302fea9d3; 
uint8_t appKey[] = { 0x07, 0xC9, 0xFE, 0x81, 0x6A, 0xD6, 0x69, 0xEB, 0x73, 0xB0, 0x46, 0x0A, 0xCE, 0x4A, 0xD9, 0x9A };
uint8_t nwkKey[] = { 0x07, 0xC9, 0xFE, 0x81, 0x6A, 0xD6, 0x69, 0xEB, 0x73, 0xB0, 0x46, 0x0A, 0xCE, 0x4A, 0xD9, 0x9A };

//Class A

/* ===================== LORa Configuration ===================== */
class LoRaP2P {
public:
    void begin();
    void send();
    void receive();
};



#endif
