#pragma once

#include <Arduino.h>
#define MAGIC_NUMBER 0xA1B2C3D8 
#define MAX_STR 32
#define MIN_STR 16
#define MAX_I2C_CHANNELS 8
#define MAX_I2C_DEVICES   8
#define MAX_MODBUS_CHANNELS 2

// ================= DEVICE =================
struct DeviceInfoCfg {
    char name[MIN_STR];
    char type[MIN_STR];
    char id[MAX_STR];
    char uid[MAX_STR];
    bool use_sd_config;
};

// ================= HARDWARE =================
struct I2CChannelCfg {
    uint8_t id;
    char name[MIN_STR];
    uint8_t reg_addr;
    uint8_t length;
    char byte_order[5];
    float scale;
};

struct I2CDeviceCfg {
    uint8_t id;
    char name[MIN_STR];
    uint8_t address;
    I2CChannelCfg channels[MAX_I2C_CHANNELS];
};

struct ModbusChannelCfg {
    uint8_t id;
    char name[MIN_STR];
    uint8_t slave_id;
    uint16_t address;
    uint8_t quantity;
    char type[10]; // HOLDING, INPUT
    char byte_order[5];
    bool sign;
};
struct I2CConfig {
    bool enable;
    uint32_t frequency;
    uint8_t master_address;
    uint32_t interval_ms;
    uint8_t max_retry;
    uint32_t max_resp_ms;
    I2CDeviceCfg devices[1]; 
};
struct HardwareCfg {
    I2CConfig i2c;


    struct { bool enabled; uint8_t address; } oled;
    struct { bool active_low; } led;
    
    struct {
        bool enable;
        float scale_min;
        float scale_max;
        float factor_slope;
        float factor_intercept;
    } analog;

    struct {
        bool enable;
        uint32_t baud_rate;
        uint8_t stop_bit;
        uint8_t data_bit;
        char parity[10];
        uint32_t interval_ms;
        uint32_t max_resp_ms;
        uint8_t max_retry;
        ModbusChannelCfg channels[MAX_MODBUS_CHANNELS];
    } modbus_rs485;

    struct {
        bool enable;
        bool inverted;
        char default_state[8];
        char mode[16];
        char pull[8];
        char speed[8];
        uint32_t startup_delay_ms;
    } ls_sw;

    struct {
        bool enable;
        char sda[5];
        char scl[5];
        uint8_t address;
    } sht3;
};

// ================= LOGGING & COMM =================
struct LoggingCfg {
    bool enabled;
    char level[8]; // INFO, DEBUG, ERROR
    bool sd_log;
};

struct CommCfg {
    struct { char port[10]; uint32_t baud; uint32_t timeout; } serial;
    struct {
        char port[10];
        uint32_t baud;
        uint32_t timeout;
        uint8_t data_bits;
        uint8_t stop_bits;
        char parity[10];
        char slaves[4][10];   
        uint8_t slave_count;
    } rs485;
};

// ================= LORA =================
struct LoRaWAN_OTAA { char join_eui[17]; char dev_eui[17]; char nwk_key[33]; char app_key[33]; };
struct LoRaWAN_ABP { char dev_addr[9]; char nwk_skey[33]; char app_skey[33]; };

struct LoRaCfg {
    bool enabled;
    char pins_ss[8]; char pins_rst[8]; char pins_dio0[8]; char pins_dio1[8];
    char region[10];
    
    struct {
        char mode[5]; // ABP, OTAA
        LoRaWAN_OTAA otaa;
        LoRaWAN_ABP abp;
        char class_type[2]; // A, B, C
        bool class_c_continuous_rx;
        uint32_t rx1_delay_ms;
        char rx1_data_rate[5];
        uint32_t rx2_frequency;
        char rx2_data_rate[5];
        uint8_t tx_sf;
        uint8_t tx_power;
        bool tx_adr;
        bool confirmed_uplink;
        uint8_t fport;
        bool duty_cycle;
        uint32_t uplink_interval_sec;
    } lorawan;
};

// ================= MAIN CONFIG =================
struct DeviceConfig {
    uint32_t magic;
    DeviceInfoCfg device;
    HardwareCfg hardware;
    LoggingCfg logging;
    CommCfg communication;
    LoRaCfg lora;
};

class ConfigManager {
public:
    bool begin();
    bool save();
    DeviceConfig& get();
    void factoryReset();
private:
    DeviceConfig config;
};