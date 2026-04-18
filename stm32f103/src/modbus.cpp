#include "mylib.h"

// ================= CONSTRUCTOR =================
ModbusRTU::ModbusRTU(HardwareSerial& serial, int dePin, int rePin) {
  _serial = &serial;
  _dePin = dePin;
  _rePin = rePin;
}

// ================= SERIAL CONFIG =================
uint32_t ModbusRTU::getSerialConfig() {
  // Determine serial mode from parity and stop bits
  if (_cfg.parity == 'E') {
    if (_cfg.stopBits == 2) return SERIAL_8E2;
    return SERIAL_8E1;
  }
  if (_cfg.parity == 'O') {
    if (_cfg.stopBits == 2) return SERIAL_8O2;
    return SERIAL_8O1;
  }
  if (_cfg.stopBits == 2) return SERIAL_8N2;
  return SERIAL_8N1;
}

// ================= BEGIN =================
void ModbusRTU::begin(ModbusConfig cfg) {
  _cfg = cfg;

  pinMode(_dePin, OUTPUT);
  if (_rePin != -1) pinMode(_rePin, OUTPUT);

  postTransmission();

  _serial->begin(_cfg.baudRate, getSerialConfig());
}

// ================= TX/RX CONTROL =================
void ModbusRTU::preTransmission() {
  digitalWrite(_dePin, HIGH);
  if (_rePin != -1) digitalWrite(_rePin, HIGH);
}

void ModbusRTU::postTransmission() {
  digitalWrite(_dePin, LOW);
  if (_rePin != -1) digitalWrite(_rePin, LOW);
}

// ================= CRC16 =================
uint16_t ModbusRTU::crc16(uint8_t* data, int length) {
  uint16_t crc = 0xFFFF;

  for (int i = 0; i < length; i++) {
    crc ^= data[i];

    for (int j = 0; j < 8; j++) {
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

// ================= SEND REQUEST =================
bool ModbusRTU::readHoldingRegisters(uint8_t slaveId, uint16_t startAddr, uint16_t quantity) {
  uint8_t frame[8];

  frame[0] = slaveId;
  frame[1] = 0x03;
  frame[2] = startAddr >> 8;
  frame[3] = startAddr & 0xFF;
  frame[4] = quantity >> 8;
  frame[5] = quantity & 0xFF;

  uint16_t crc = crc16(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = crc >> 8;

  preTransmission();
  delayMicroseconds(100);

  _serial->write(frame, 8);
  _serial->flush();

  delayMicroseconds(100);
  postTransmission();

  return true;
}

// ================= READ RESPONSE =================
int ModbusRTU::readResponse(uint8_t* buffer, int maxLen) {
  int i = 0;
  unsigned long start = millis();

  while (millis() - start < _cfg.maxRespTime) {
    if (_serial->available()) {
      buffer[i++] = _serial->read();
      if (i >= maxLen) break;
    }
  }

  if (i < 5) return -1;

  uint16_t crcCalc = crc16(buffer, i - 2);
  uint16_t crcRecv = buffer[i - 2] | (buffer[i - 1] << 8);

  if (crcCalc != crcRecv) return -2;

  return i;
}

// ================= SCHEDULER =================
bool ModbusRTU::update(uint8_t slaveId, uint16_t startAddr, uint16_t quantity) {
  if (!_cfg.enable) return false;

  if (millis() - _lastExec < _cfg.execInterval) return false;
  _lastExec = millis();

  uint8_t buf[32];

  int retry = 0;

  while (retry < _cfg.maxRetry) {

    readHoldingRegisters(slaveId, startAddr, quantity);

    int len = readResponse(buf, sizeof(buf));

    if (len > 0) {
      Serial.println("OK");
      return true;
    }

    retry++;
  }

  Serial.println("Timeout");
  return false;
}

// ================= PASS THROUGH =================
void ModbusRTU::handlePassThrough(Stream& uplink) {
  if (!_cfg.passThrough) return;

  // uplink → RS485
  if (uplink.available()) {
    uint8_t buf[64];
    int len = uplink.readBytes(buf, 64);

    preTransmission();
    _serial->write(buf, len);
    _serial->flush();
    postTransmission();
  }

  // RS485 → uplink
  if (_serial->available()) {
    uint8_t buf[64];
    int len = _serial->readBytes(buf, 64);

    uplink.write(buf, len);
  }
}

// ================= HELPER =================
int ModbusRTU::available() {
  return _serial->available();
}