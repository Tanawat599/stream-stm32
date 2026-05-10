#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "mylib.h"
#include <Adafruit_SHTC3.h>


Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();
MySHTC3::MySHTC3(TwoWire* wire, uint8_t sda, uint8_t scl) {

    _wire = wire;

    _sda = sda;
    _scl = scl;

    _temperature = 0;
    _humidity = 0;
}

bool MySHTC3::begin() {

    _wire->setSDA(_sda);
    _wire->setSCL(_scl);

    _wire->begin();

    if (!_shtc3.begin()) {
        return false;
    }

    return true;
}

bool MySHTC3::read() {

    sensors_event_t humidity;
    sensors_event_t temp;

    if (!_shtc3.getEvent(&humidity, &temp)) {
        return false;
    }

    _temperature = temp.temperature;
    _humidity = humidity.relative_humidity;

    return true;
}


float MySHTC3::getTemperature() {
    return _temperature;
}

float MySHTC3::getHumidity() {
    return _humidity;
}