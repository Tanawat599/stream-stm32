#pragma once

#include <Arduino.h>

// ======================================================
// DEVICE INFO
// ======================================================

struct DeviceInfo {

    char name[32];
    char type[32];
    char id[32];
    char uid[32];
};

// ======================================================
// I2C
// ======================================================

struct I2CChannelConfig {

    uint8_t id;
    char name[32];
    uint16_t reg;
    uint8_t length;
    char byteOrder[8];
    char type[16];
    float scale;
};

struct I2CDeviceConfig {

    uint8_t id;
    char name[32];
    uint8_t address;
    uint8_t channelCount;
    I2CChannelConfig channels[16];
};

struct I2CConfig {

    bool enable;
    uint32_t frequency;
    uint8_t masterAddress;
    uint32_t intervalMs;
    uint32_t maxRespMs;
    uint8_t maxRetry;
    uint8_t deviceCount;
    I2CDeviceConfig devices[16];
};

// ======================================================
// OLED
// ======================================================

struct OLEDConfig {

    bool enable;
    char sda[8];
    char scl[8];
    uint8_t address;
};

// ======================================================
// LED
// ======================================================

struct LEDConfig {
    bool activeLow;
};

// ======================================================
// ANALOG
// ======================================================

struct AnalogScaleConfig {

    float min;
    float max;
};

struct AnalogFactorConfig {

    float slope;
    float intercept;
};

struct AnalogConfig {

    bool enable;
    AnalogScaleConfig scale;
    AnalogFactorConfig factor;
};

// ======================================================
// MODBUS
// ======================================================

struct ModbusChannelConfig {

    uint8_t id;
    char name[32];
    uint8_t slaveId;
    uint16_t address;
    uint8_t quantity;
    char type[16];
    char byteOrder[8];
    bool sign;
};

struct ModbusRS485Config {

    bool enable;
    uint32_t baudRate;
    uint8_t stopBit;
    uint8_t dataBit;
    char parity[8];
    uint32_t intervalMs;
    uint32_t maxRespMs;
    uint8_t maxRetry;
    uint8_t channelCount;
    ModbusChannelConfig channels[32];
};

// ======================================================
// LOW SIDE SWITCH
// ======================================================

struct LowSideSwitchConfig {

    bool enable;
    bool inverted;
    char defaultState[8];
    char mode[16];
    char pull[16];
    char speed[16];
    uint32_t startupDelayMs;
};

// ======================================================
// SHT3
// ======================================================

struct SHT3Config {

    bool enable;
    char sda[8];
    char scl[8];
    uint8_t address;
};

// ======================================================
// HARDWARE
// ======================================================

struct HardwareConfig {

    I2CConfig i2c;
    OLEDConfig oled;
    LEDConfig led;
    AnalogConfig analog;
    ModbusRS485Config modbus;
    LowSideSwitchConfig lsSw;
    SHT3Config sht3;
};

// ======================================================
// LOGGING
// ======================================================

struct LoggingConfig {

    bool enabled;
    char level[16];
    bool sdLog;
};

// ======================================================
// SERIAL COMM
// ======================================================

struct SerialCommConfig {

    char port[16];
    uint32_t baud;
    uint32_t timeout;
};

// ======================================================
// RS485 COMM
// ======================================================

struct RS485FrameConfig {

    uint8_t dataBits;
    uint8_t stopBits;
    char parity[8];
};

struct RS485CommConfig {

    char port[16];
    uint32_t baud;
    uint32_t timeout;
    RS485FrameConfig frame;
    uint8_t slaveCount;
    uint8_t slaves[16];
};

// ======================================================
// COMMUNICATION
// ======================================================

struct CommunicationConfig {

    SerialCommConfig serial;
    RS485CommConfig rs485;
};

// ======================================================
// LORA PINS
// ======================================================

struct LoRaPinConfig {

    char ss[8];
    char rst[8];
    char dio0[8];
    char dio1[8];
};

// ======================================================
// OTAA
// ======================================================

struct OTAAConfig {

    char joinEui[32];
    char devEui[32];
    char nwkKey[64];
    char appKey[64];
};

// ======================================================
// ABP
// ======================================================

struct ABPConfig {

    char devAddr[16];
    char nwkSKey[64];
    char appSKey[64];
};

// ======================================================
// CLASS A
// ======================================================

struct ClassAConfig {

    uint32_t rx1DelayMs;
    char rx1DataRate[16];
    uint32_t uplinkIntervalMin;
};

// ======================================================
// CLASS C
// ======================================================

struct ClassCConfig {

    bool continuousRx;
};

// ======================================================
// CLASS
// ======================================================

struct LoRaClassConfig {

    char classType[8];
    ClassAConfig classA;
    ClassCConfig classC;
};

// ======================================================
// RX2
// ======================================================

struct RX2Config {

    uint32_t frequency;
    char dataRate[16];
};

// ======================================================
// TX
// ======================================================

struct TXConfig {

    uint8_t sf;
    int8_t power;
    bool adr;
};

// ======================================================
// LORAWAN
// ======================================================

struct LoRaWANConfig {

    char mode[8];
    OTAAConfig otaa;
    ABPConfig abp;
    LoRaClassConfig classConfig;
    uint32_t uplinkIntervalSec;
    RX2Config rx2;
    TXConfig tx;
    bool confirmedUplink;
    uint8_t fport;
    bool dutyCycle;
};

// ======================================================
// LORA
// ======================================================

struct LoRaConfig {

    bool enabled;
    LoRaPinConfig pins;
    char region[16];
    LoRaWANConfig lorawan;
};

// ======================================================
// ROOT CONFIG
// ======================================================

struct DeviceConfig {

    DeviceInfo device;
    HardwareConfig hardware;
    LoggingConfig logging;
    CommunicationConfig communication;
    LoRaConfig lora;
};