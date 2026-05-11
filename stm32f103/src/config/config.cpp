#include "mylib.h"
#include "device_config.h"
#include <EEPROM.h>

#define EEPROM_SIZE  4096

bool ConfigManager::begin() {
    EEPROM.begin();
    EEPROM.get(0, config);

    if (config.magic != MAGIC_NUMBER) {
        Serial1.println(F("[CONFIG] Magic mismatch, factory resetting..."));
        factoryReset();
        save();
        return false;
    }
    if (config.lora.lorawan.uplink_interval_sec > 86400) {  
        Serial1.println(F("[CONFIG] Invalid interval, factory reset again"));
        factoryReset();
        save();
        return false;
    }

    return true;
}

void ConfigManager::factoryReset() {
    memset(&config, 0, sizeof(DeviceConfig));
    config.magic = MAGIC_NUMBER;

    // --- Device ---
    strcpy(config.device.name, "STM32F103");
    strcpy(config.device.type, "lorawan_node");
    strcpy(config.device.id, "stm32f103_node_001");
    strcpy(config.device.uid, "66EFF303436474257135038");

    // --- Hardware : I2C ---
    config.hardware.i2c.enable = true;
    config.hardware.i2c.frequency = 100000;
    config.hardware.i2c.master_address = 0x08;
    config.hardware.i2c.interval_ms = 2000;
    config.hardware.i2c.max_resp_ms = 1000;
    config.hardware.i2c.max_retry = 3;

    config.hardware.i2c.devices[0].id = 1;
    strcpy(config.hardware.i2c.devices[0].name, "sensor_1");
    config.hardware.i2c.devices[0].address = 0x40;
    
    // I2C Channel 1
    config.hardware.i2c.devices[0].channels[0].id = 1;
    strcpy(config.hardware.i2c.devices[0].channels[0].name, "temperature");
    config.hardware.i2c.devices[0].channels[0].reg_addr = 0x00;
    config.hardware.i2c.devices[0].channels[0].length = 2;
    strcpy(config.hardware.i2c.devices[0].channels[0].byte_order, "AB");
    config.hardware.i2c.devices[0].channels[0].scale = 0.1;

    // --- Hardware : OLED & LED ---
    config.hardware.oled.enabled = true;
    config.hardware.oled.address = 0x3C;
    config.hardware.led.active_low = false;

    // --- Hardware : Analog ---
    config.hardware.analog.enable = true;
    config.hardware.analog.scale_min = 4;
    config.hardware.analog.scale_max = 20;
    config.hardware.analog.factor_slope = 1.0;
    config.hardware.analog.factor_intercept = 0.0;

    // --- Hardware : Modbus ---
    config.hardware.modbus_rs485.enable = true;
    config.hardware.modbus_rs485.baud_rate = 9600;
    config.hardware.modbus_rs485.data_bit = 8;
    config.hardware.modbus_rs485.stop_bit = 1;
    strcpy(config.hardware.modbus_rs485.parity, "NONE");
    config.hardware.modbus_rs485.interval_ms = 2000;
    config.hardware.modbus_rs485.max_resp_ms = 1000;
    config.hardware.modbus_rs485.max_retry = 3;
    
    config.hardware.modbus_rs485.channels[0].id = 1;
    strcpy(config.hardware.modbus_rs485.channels[0].name, "temperature");
    config.hardware.modbus_rs485.channels[0].slave_id = 1;
    config.hardware.modbus_rs485.channels[0].address = 0;
    config.hardware.modbus_rs485.channels[0].quantity = 1;
    strcpy(config.hardware.modbus_rs485.channels[0].type, "HOLDING");
    strcpy(config.hardware.modbus_rs485.channels[0].byte_order, "AB");

    // --- Ls Switch ---
    config.hardware.ls_sw.enable = true;
    config.hardware.ls_sw.inverted = false;
    strcpy(config.hardware.ls_sw.default_state, "off");
    strcpy(config.hardware.ls_sw.mode, "push_pull");
    strcpy(config.hardware.ls_sw.pull, "none");
    strcpy(config.hardware.ls_sw.speed, "low");
    config.hardware.ls_sw.startup_delay_ms = 10;

    // --- SHT3 ---
    config.hardware.sht3.enable = true;
    config.hardware.sht3.address = 0x44;

    // --- Logging & Comm ---
    config.logging.enabled = true;
    strcpy(config.logging.level, "INFO");
    config.logging.sd_log = true;

    strcpy(config.communication.serial.port, "Serial1");
    config.communication.serial.baud = 115200;
    config.communication.serial.timeout = 1000;

    // --- LoRaWAN ---
    config.lora.enabled = true;
    strcpy(config.lora.pins_ss, "PB11");
    strcpy(config.lora.pins_rst, "PB12");
    strcpy(config.lora.pins_dio0, "PB0");
    strcpy(config.lora.region, "AS923");

    strcpy(config.lora.lorawan.mode, "ABP");
    strcpy(config.lora.lorawan.class_type, "C");
    config.lora.lorawan.class_c_continuous_rx = true;
    config.lora.lorawan.uplink_interval_sec = 15;
    config.lora.lorawan.tx_sf = 7;
    config.lora.lorawan.tx_power = 14;
    config.lora.lorawan.tx_adr = true;
    config.lora.lorawan.duty_cycle = true;
    config.lora.lorawan.fport = 2;
    config.lora.lorawan.rx1_delay_ms = 500;
    strcpy(config.lora.lorawan.rx1_data_rate, "DR5");
    config.lora.lorawan.rx2_frequency = 923200000;
    strcpy(config.lora.lorawan.rx2_data_rate, "DR2");
    config.lora.lorawan.confirmed_uplink = false;

    strcpy(config.lora.lorawan.abp.dev_addr, "01b3bb05");
    strcpy(config.lora.lorawan.abp.nwk_skey, "9603718ec7d6a70fd78ab4daeb5c9224");
    strcpy(config.lora.lorawan.abp.app_skey, "af550b69d4c6e505fada5c6ea77953e8");
    
    strcpy(config.lora.lorawan.otaa.dev_eui, "C304DB83070E0063");
    strcpy(config.lora.lorawan.otaa.app_key, "28A77CA4837A951F42A2D7A31493043C");
    strcpy(config.lora.lorawan.otaa.join_eui, "E63BA610B2498DBE");
    strcpy(config.lora.lorawan.otaa.nwk_key, "28A77CA4837A951F42A2D7A31493043C");

    save();
}

bool ConfigManager::save() {
    EEPROM.put(0, config);
    return true;
}

DeviceConfig& ConfigManager::get() { return config; }