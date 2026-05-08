#include "mylib.h"
#include "device_config.h"
// ======================================================
// FLASH OVERRIDE STRUCT
// ======================================================

#define CFG_MAGIC 0xDEADBEEF

typedef struct {

    uint32_t magic;

    uint32_t uplinkIntervalSec;

    bool oledEnable;

    bool i2cEnable;

    bool modbusEnable;

    bool sht3Enable;

} FlashConfig;

// ======================================================
// LOAD CONFIG
// ======================================================

bool ConfigManager::load(
    const char* path
) {

    EEPROM.begin();

    File file = SD.open(path);

    if (!file) {

        Serial1.println(
            "[CFG] Open config failed"
        );

        return false;
    }

    //StaticJsonDocument<8192> doc;
    JsonDocument doc;

    auto err = deserializeJson(doc, file);

    file.close();

    if (err) {

        Serial1.print(
            "[CFG] JSON Error: "
        );

        Serial1.println(err.c_str());

        return false;
    }

    // ==================================================
    // DEVICE
    // ==================================================

    strlcpy(
        config.device.name,
        doc["device"]["name"] | "",
        sizeof(config.device.name)
    );

    strlcpy(
        config.device.type,
        doc["device"]["type"] | "",
        sizeof(config.device.type)
    );

    strlcpy(
        config.device.id,
        doc["device"]["id"] | "",
        sizeof(config.device.id)
    );

    strlcpy(
        config.device.uid,
        doc["device"]["uid"] | "",
        sizeof(config.device.uid)
    );

    // ==================================================
    // HARDWARE
    // ==================================================

    JsonObject hw = doc["hardware"];

    // ================= I2C =================

    config.hardware.i2c.enable =
        hw["i2c"]["enable"] | false;

    config.hardware.i2c.frequency =
        hw["i2c"]["frequency"] | 100000;

    config.hardware.i2c.masterAddress =
        strtol(
            hw["i2c"]["master_address"] | "0x08",
            NULL,
            16
        );

    config.hardware.i2c.intervalMs =
        hw["i2c"]["interval_ms"] | 2000;

    config.hardware.i2c.maxRespMs =
        hw["i2c"]["max_resp_ms"] | 1000;

    config.hardware.i2c.maxRetry =
        hw["i2c"]["max_retry"] | 3;

    // ================= OLED =================

    config.hardware.oled.enable =
        hw["oled"]["enabled"] |
        hw["oled"]["enable"] |
        false;

    config.hardware.oled.address =
        strtol(
            hw["oled"]["address"] | "0x3C",
            NULL,
            16
        );

    // ================= ANALOG =================

    config.hardware.analog.enable =
        hw["analog"]["enable"] |
        false;

    config.hardware.analog.scale.min =
        hw["analog"]["scale"]["min"] |
        4.0;

    config.hardware.analog.scale.max =
        hw["analog"]["scale"]["max"] |
        20.0;

    config.hardware.analog.factor.slope =
        hw["analog"]["factor"]["slope"] |
        1.0;

    config.hardware.analog.factor.intercept =
        hw["analog"]["factor"]["intercept"] |
        0.0;

    // ================= MODBUS =================

    config.hardware.modbus.enable =
        hw["modbus_rs485"]["enable"] |
        false;

    config.hardware.modbus.baudRate =
        hw["modbus_rs485"]["baud_rate"] |
        9600;

    config.hardware.modbus.stopBit =
        hw["modbus_rs485"]["stop_bit"] |
        1;

    config.hardware.modbus.dataBit =
        hw["modbus_rs485"]["data_bit"] |
        8;

    strlcpy(
        config.hardware.modbus.parity,
        hw["modbus_rs485"]["parity"] |
        "NONE",
        sizeof(config.hardware.modbus.parity)
    );

    // ================= SHT3 =================

    config.hardware.sht3.enable =
        hw["sht3"]["enable"] |
        false;

    config.hardware.sht3.address =
        strtol(
            hw["sht3"]["address"] | "0x44",
            NULL,
            16
        );

    // ==================================================
    // LORA
    // ==================================================

    JsonObject lora = doc["lora"];

    config.lora.enabled =
        lora["enabled"] |
        true;

    strlcpy(
        config.lora.region,
        lora["region"] |
        "AS923",
        sizeof(config.lora.region)
    );

    config.lora.lorawan.uplinkIntervalSec =
        lora["lorawan"]
            ["uplink_interval_sec"] |
        60;

    config.lora.lorawan.tx.sf =
        lora["lorawan"]["tx"]["sf"] |
        7;

    config.lora.lorawan.tx.power =
        lora["lorawan"]["tx"]["power"] |
        14;

    config.lora.lorawan.tx.adr =
        lora["lorawan"]["tx"]["adr"] |
        true;

    // ==================================================
    // LOAD EEPROM OVERRIDE
    // ==================================================

    loadOverrides();

    Serial1.println(
        "[CFG] Config loaded"
    );

    return true;
}

// ======================================================
// LOAD EEPROM OVERRIDE
// ======================================================

void ConfigManager::loadOverrides() {

    FlashConfig fc;

    EEPROM.get(0, fc);

    if (fc.magic != CFG_MAGIC) {

        Serial1.println(
            "[CFG] No EEPROM override"
        );

        return;
    }

    config.lora.lorawan.uplinkIntervalSec =
        fc.uplinkIntervalSec;

    config.hardware.oled.enable =
        fc.oledEnable;

    config.hardware.i2c.enable =
        fc.i2cEnable;

    config.hardware.modbus.enable =
        fc.modbusEnable;

    config.hardware.sht3.enable =
        fc.sht3Enable;

    Serial1.println(
        "[CFG] EEPROM override loaded"
    );
}

// ======================================================
// SAVE EEPROM
// ======================================================

bool ConfigManager::save() {

    FlashConfig fc;

    fc.magic = CFG_MAGIC;

    fc.uplinkIntervalSec =
        config.lora.lorawan
            .uplinkIntervalSec;

    fc.oledEnable =
        config.hardware.oled.enable;

    fc.i2cEnable =
        config.hardware.i2c.enable;

    fc.modbusEnable =
        config.hardware.modbus.enable;

    fc.sht3Enable =
        config.hardware.sht3.enable;
    EEPROM.put(0, fc);


    Serial1.println(
        "[CFG] EEPROM saved"
    );

    return true;
}

// ======================================================
// SET VALUE
// ======================================================

bool ConfigManager::setValue(
    const char* key,
    const char* value
) {

    // ================= LORA =================

    if (strcmp(
        key,
        "lora.interval"
    ) == 0) {

        config.lora.lorawan
            .uplinkIntervalSec =
            atoi(value);

        return true;
    }

    // ================= OLED =================

    if (strcmp(
        key,
        "oled.enable"
    ) == 0) {

        config.hardware.oled.enable =
            strcmp(value, "true") == 0;

        return true;
    }

    // ================= I2C =================

    if (strcmp(
        key,
        "i2c.enable"
    ) == 0) {

        config.hardware.i2c.enable =
            strcmp(value, "true") == 0;

        return true;
    }

    // ================= MODBUS =================

    if (strcmp(
        key,
        "modbus.enable"
    ) == 0) {

        config.hardware.modbus.enable =
            strcmp(value, "true") == 0;

        return true;
    }

    // ================= SHT3 =================

    if (strcmp(
        key,
        "sht3.enable"
    ) == 0) {

        config.hardware.sht3.enable =
            strcmp(value, "true") == 0;

        return true;
    }

    return false;
}

// ======================================================
// GET VALUE
// ======================================================

String ConfigManager::getValue(
    const char* key
) {

    if (strcmp(
        key,
        "lora.interval"
    ) == 0) {

        return String(
            config.lora.lorawan
                .uplinkIntervalSec
        );
    }

    if (strcmp(
        key,
        "oled.enable"
    ) == 0) {

        return config.hardware.oled.enable
            ? "true"
            : "false";
    }

    return "unknown";
}

// ======================================================
// PRINT CONFIG
// ======================================================

void ConfigManager::print(
    Stream& serial
) {

    serial.println();
    serial.println(
        "========== CONFIG =========="
    );

    serial.print("Device: ");
    serial.println(
        config.device.name
    );

    serial.print(
        "LoRa Interval: "
    );

    serial.println(
        config.lora.lorawan
            .uplinkIntervalSec
    );

    serial.print("OLED: ");

    serial.println(
        config.hardware.oled.enable
            ? "ON"
            : "OFF"
    );

    serial.print("I2C: ");

    serial.println(
        config.hardware.i2c.enable
            ? "ON"
            : "OFF"
    );

    serial.print("MODBUS: ");

    serial.println(
        config.hardware.modbus.enable
            ? "ON"
            : "OFF"
    );

    serial.print("SHT3: ");

    serial.println(
        config.hardware.sht3.enable
            ? "ON"
            : "OFF"
    );

    serial.println(
        "============================"
    );
}

void ConfigManager::clearOverrides() {

    FlashConfig fc;

    memset(&fc, 0, sizeof(fc));

    EEPROM.put(0, fc);

    Serial1.println(
        "[CFG] EEPROM cleared"
    );
}
void ConfigManager::apply() {

    // OLED
    if(config.hardware.oled.enable) {

        Serial1.println(
            "[APPLY] OLED ENABLED"
        );

    } else {

        Serial1.println(
            "[APPLY] OLED DISABLED"
        );
    }

    // I2C
    if(config.hardware.i2c.enable) {

        Serial1.println(
            "[APPLY] I2C ENABLED"
        );

    } else {

        Serial1.println(
            "[APPLY] I2C DISABLED"
        );
    }

    // MODBUS
    if(config.hardware.modbus.enable) {

        Serial1.println(
            "[APPLY] MODBUS ENABLED"
        );

    } else {

        Serial1.println(
            "[APPLY] MODBUS DISABLED"
        );
    }
}