#include "mylib.h"



Analog420::Analog420() {}

void Analog420::begin(AnalogConfig cfg) {
  _cfg = cfg;
  pinMode(_cfg.pin, INPUT);
}

// ================= RAW ADC =================
float Analog420::readRaw() {
  return analogRead(_cfg.pin);
}

// ================= VOLTAGE =================
float Analog420::readVoltage() {
  float raw = readRaw();
  return (raw / _cfg.adcResolution) * _cfg.vref;
}

// ================= CURRENT (mA) =================
float Analog420::readCurrent() {
  float voltage = readVoltage();

  float current = (voltage / _cfg.shuntResistor) * 1000.0;

  return current;
}

// ================= SCALED VALUE =================
float Analog420::readScaled() {
  float current = readCurrent();

  if (current < _cfg.minCurrent) current = _cfg.minCurrent;
  if (current > _cfg.maxCurrent) current = _cfg.maxCurrent;

  float ratio = (current - _cfg.minCurrent) / (_cfg.maxCurrent - _cfg.minCurrent);

  float value = _cfg.outMin + ratio * (_cfg.outMax - _cfg.outMin);

  value *= _cfg.multiplier;

  return value;
}