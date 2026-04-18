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

uint16_t modbusCRC(uint8_t* buf, int len) {
  uint16_t crc = 0xFFFF;

  for (int pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];

    for (int i = 0; i < 8; i++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void RS485::modbusReadHolding(uint8_t slaveId, uint16_t startAddr, uint16_t quantity) {
  uint8_t frame[8];

  frame[0] = slaveId;
  frame[1] = 0x03;
  frame[2] = startAddr >> 8;
  frame[3] = startAddr & 0xFF;
  frame[4] = quantity >> 8;
  frame[5] = quantity & 0xFF;

  uint16_t crc = modbusCRC(frame, 6);
  frame[6] = crc & 0xFF;       // CRC Low
  frame[7] = crc >> 8;         // CRC High

  sendBytes(frame, 8);
}

int RS485::modbusReceive(uint8_t* buffer, size_t len) {
  int n = receiveBytes(buffer, len);

  if (n < 5) return -1; 

  uint16_t crcCalc = modbusCRC(buffer, n - 2);
  uint16_t crcRecv = buffer[n - 2] | (buffer[n - 1] << 8);

  if (crcCalc != crcRecv) {
    return -2; 
  }

  return n;
}