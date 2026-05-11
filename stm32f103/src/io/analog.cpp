#include "mylib.h"
#include "config.h"

Analog420::Analog420() {}

// ================= BEGIN =================
void Analog420::begin() {
    pinMode(ANALOG_PIN, INPUT);
}

// ================= LOAD CONFIG FROM STRUCT (EEPROM) =================
void Analog420::loadConfigFromStruct(const HardwareCfg& hw) {
    const auto& analog = hw.analog;

    // คัดลอกเฉพาะฟิลด์ที่มีใน HardwareCfg
    _cfg.enable            = analog.enable;
    _cfg.scale_min         = analog.scale_min;
    _cfg.scale_max         = analog.scale_max;
    _cfg.factor_slope      = analog.factor_slope;
    _cfg.factor_intercept  = analog.factor_intercept;

    // adc/vref/shunt resistor มาจาก default หรืออาจเพิ่มใน HardwareCfg ภายหลัง
    // _cfg.adc_resolution = 4095.0; (ไม่ต้องเปลี่ยน)

    Serial1.println(F("[Analog] Loaded from EEPROM Struct"));
}

// ================= LOAD CONFIG FROM JSON (SD Card) =================
void Analog420::loadConfigFromJson(const JsonObject& json) {
    _cfg.enable = json["enable"] | true;

    // ใช้ is<T>() แทน containsKey เพื่อหลีกเลี่ยง deprecated warning
    if (json["scale"].is<JsonObject>()) {
        JsonObject scale = json["scale"];
        _cfg.scale_min = scale["min"] | 4.0f;
        _cfg.scale_max = scale["max"] | 20.0f;
    }

    if (json["factor"].is<JsonObject>()) {
        JsonObject factor = json["factor"];
        _cfg.factor_slope     = factor["slope"] | 1.0f;
        _cfg.factor_intercept = factor["intercept"] | 0.0f;
    }

    // Optional hardware settings
    _cfg.adc_resolution = json["adc_resolution"] | 4095.0f;
    _cfg.vref           = json["vref"] | 3.3f;
    _cfg.shunt_resistor = json["shunt_resistor"] | 150.0f;

    Serial1.println(F("[Analog] Loaded from JSON"));
}

// ================= RAW ADC =================
float Analog420::readRaw() {
    return analogRead(ANALOG_PIN);
}

// ================= VOLTAGE =================
float Analog420::readVoltage() {
    float raw = readRaw();
    return (raw / _cfg.adc_resolution) * _cfg.vref;
}

// ================= CURRENT (mA) =================
float Analog420::readCurrent() {
    float voltage = readVoltage();
    return (voltage / _cfg.shunt_resistor) * 1000.0;
}

// ================= SCALED VALUE =================
float Analog420::readScaled() {
    float current = readCurrent();

    // Clamp to 4-20 mA range
    if (current < _cfg.scale_min) current = _cfg.scale_min;
    if (current > _cfg.scale_max) current = _cfg.scale_max;

    // Linear scaling: scaled = slope * current + intercept
    return _cfg.factor_slope * current + _cfg.factor_intercept;
}