#include "config.h"
#include "mylib.h"
#include "Arduino.h"

RS485::RS485() {
  _serial = &RS485_SERIAL;
  _dePin = RS485_DE_PIN;
  _rePin = RS485_RE_PIN;
}

void RS485::begin(long baud) {
  pinMode(_dePin, OUTPUT);
  if (_rePin != -1) pinMode(_rePin, OUTPUT);

  setReceive();
  _serial->begin(baud);
}

void RS485::setTransmit() {
  digitalWrite(_dePin, HIGH);
  if (_rePin != -1) digitalWrite(_rePin, HIGH);
}

void RS485::setReceive() {
  digitalWrite(_dePin, LOW);
  if (_rePin != -1) digitalWrite(_rePin, LOW);
}

// ===== Send =====
void RS485::send(String data) {
  setTransmit();
  delayMicroseconds(50);

  _serial->println(data);
  _serial->flush();

  delayMicroseconds(50);
  setReceive();
}

void RS485::sendBytes(uint8_t* data, size_t len) {
  setTransmit();
  delayMicroseconds(50);

  _serial->write(data, len);
  _serial->flush();

  delayMicroseconds(50);
  setReceive();
}

// ===== Receive =====
String RS485::receive() {
  return _serial->readStringUntil('\n');
}

int RS485::receiveBytes(uint8_t* buffer, size_t len) {
  return _serial->readBytes(buffer, len);
}

bool RS485::available() {
  return _serial->available();
}