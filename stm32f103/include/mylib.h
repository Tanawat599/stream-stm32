#ifndef MYLIB_H
#define MYLIB_H
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <vector>




// ===================== LoRa P2P =====================
class LoRaP2P {
public:
  void begin(float frequency);
  void loadConfig(class SDResourceManager& sd, const char* path = "/CONFIG~1.JSO");
  void send(const char* msg);
  void sendBytes(uint8_t* data, size_t len);

  String receive();
  int receiveBytes(uint8_t* buffer, size_t len);
};

// ===================== LoRa WAN =====================


enum LoRaClassMode {
  CLASS_A,
  CLASS_C
};

class LoRaWan {
public:
  void begin();
  void loop(const char* payload);
  void loadConfig(class SDResourceManager& sd, const char* path = "/CONFIG~1.JSO");
  void setMode(LoRaClassMode mode);  
  void classC();
  void classA();
  void end();

private:
  LoRaClassMode currentMode = CLASS_C; 
};
class SDResourceManager {
public:
    SDResourceManager(uint8_t mosi, uint8_t miso, uint8_t sck, uint8_t cs);

    bool begin();
    void listFiles(Stream& serial, const char* dirName = "/", int numTabs = 0);

    String readFile(const char* path);
    bool loadConfig(const char* path);

    String getLoRaKey() { return _loraKey; }
    int getSensorPin() { return _pin; }

    const char* getConfig();

    bool writeLog(const char* message);
    void end();

private:
    uint8_t _mosi, _miso, _sck, _cs;

    String _loraKey;
    int _pin;
};
class Logger {
private:
    SDResourceManager* _sd;

public:
    Logger(SDResourceManager* sd);

    void logKV(const char* type, int count, ...);
    void logMsg(const char* type, const char* message);
    void logMixed(const char* type, const char* message, int count, ...);
};
// ===================== Display =====================
// class Display {
// public:
//   void begin(uint8_t address);

//   void clear();
//   void setCursor(uint8_t x, uint8_t y);

//   void print(const char* msg);
//   void printAt(uint8_t x, uint8_t y, const char* msg);

//   void update();

// private:
//   class Adafruit_SSD1306* _display; // forward declaration
// };
class OLED {
public:
    void begin();

    void clear();
    void update();
    void loadConfig(SDResourceManager& sd, const char* path = "/CONFIG~1.JSO");

    // ---------------- TEXT ----------------
    void setFont(const uint8_t* font);
    void print(const char* text, int x, int y); 
    void println(const char* text, int x, int y);   
    void drawStr(int x, int y, const char* text);

    // ---------------- DRAW ----------------
    void drawPixel(int x, int y);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawBox(int x, int y, int w, int h);
    void drawFrame(int x, int y, int w, int h);

    // ---------------- SETTINGS ----------------
    void setContrast(uint8_t value);
    void setFlip(bool flip);

    // ---------------- TEST ----------------
    void test();
};

// ===================== I2C =====================


struct I2C_Channel {
    String name;
    uint8_t reg;
    uint8_t length;
    String byte_order;
    float scale;
};

struct I2C_Device {
    String name;
    uint8_t address;
    std::vector<I2C_Channel> channels; 
};

class I2C {
public:
    void loadConfig(SDResourceManager& sd, const char* path);
    void master_begin();
    void master_loop();
    void slave_begin(uint8_t address);
    void slave_loop();

private:
    bool _enabled = false;
    uint32_t _frequency = 100000;  
    uint32_t _interval_ms = 2000;
    uint32_t _lastPoll = 0;
    uint8_t _max_retry = 3;
    
    std::vector<I2C_Device> _devices; 
    // Helper functions
    bool readRegister(uint8_t devAddr, uint8_t regAddr, uint8_t* buffer, uint8_t len);
    uint32_t processRawData(uint8_t* data, uint8_t len, String order);
    uint8_t parseHex(const char* str);

    // Slave static variables
    static volatile bool _newData;
    static char _buffer[32];
    static volatile int _idx;
    static void receiveEvent(int howMany);
    static void requestEvent();

    static volatile uint8_t _currentRegister; 
    static uint8_t _registers[16]; 
};

// ===================== RS485 =====================
class RS485 {
public:
  RS485();

  // ===== Basic =====
  void begin(long baud);
  void setTransmit();
  void setReceive();

  void send(String data);
  void sendBytes(uint8_t* data, size_t len);

  String receive();
  int receiveBytes(uint8_t* buffer, size_t len);
  bool available();

  // ===== Modbus =====
  void modbusReadHolding(uint8_t slaveId, uint16_t startAddr, uint16_t quantity);
  int modbusReceive(uint8_t* buffer, size_t len);

private:
  HardwareSerial* _serial;
  int _dePin;
  int _rePin;

  // ===== Internal =====
  uint16_t modbusCRC(uint8_t* buf, int len);
};

struct ModbusConfig {
  bool enable;

  uint32_t baudRate;
  char parity;      // 'N','E','O'
  uint8_t stopBits; // 1,2

  uint32_t execInterval; // ms
  uint32_t maxRespTime;  // ms
  uint8_t maxRetry;

  bool passThrough;
};

class ModbusRTU {
public:
  ModbusRTU(HardwareSerial& serial, int dePin, int rePin = -1);

  void begin(ModbusConfig cfg);

  // Master function
  bool readHoldingRegisters(uint8_t slaveId, uint16_t startAddr, uint16_t quantity);

  // Scheduler (auto interval + retry)
  bool update(uint8_t slaveId, uint16_t startAddr, uint16_t quantity);

  // Pass-through (RS485 ↔ uplink เช่น LoRa / Serial)
  void handlePassThrough(Stream& uplink);

  int available();

private:
  HardwareSerial* _serial;
  int _dePin;
  int _rePin;

  ModbusConfig _cfg;
  unsigned long _lastExec = 0;

  void preTransmission();
  void postTransmission();

  int readResponse(uint8_t* buffer, int maxLen);
  uint16_t crc16(uint8_t* data, int length);

  uint32_t getSerialConfig();
};

struct AnalogConfig {
  uint8_t pin;

  float vref;        
  int adcResolution; 

  float shuntResistor; 

  float minCurrent; 
  float maxCurrent; 

  float outMin; 
  float outMax; 

  float multiplier; 
};

// ================= CLASS =================
class Analog420 {
public:
  Analog420();

  void begin(AnalogConfig cfg);

  float readCurrent();   // mA
  float readVoltage();   // V
  float readRaw();       // ADC
  float readScaled();    // ค่าใช้งานจริง (มี multiplier)

private:
  AnalogConfig _cfg;
};

// ===================== RS485 =====================
class SDResourceManager;

// เติม Prefix เพื่อป้องกันการชนกับ Macro ของระบบ
enum PARITY_OPT { MB_PARITY_NONE = 0, MB_PARITY_ODD, MB_PARITY_EVEN };
enum MODBUS_TYPE { MB_COIL = 1, MB_DISCRETE = 2, MB_HOLDING = 3, MB_INPUT = 4 };
enum MB_BYTE_ORDER { BO_AB = 0, BO_BA, BO_ABCD, BO_CDBA, BO_BADC, BO_DCBA };

struct RS485_CONF {
    bool ENABLE;
    uint8_t STOP;
    uint8_t DATA;
    PARITY_OPT PARITY;
    uint32_t BAUD;
    uint32_t INTERVAL;
    uint32_t MAX_RESP;
    uint8_t MAX_RETRY;
};

struct MODBUS_CH {
    uint8_t CH_ID;
    char NAME[16];
    uint8_t SLAVE;
    uint16_t ADDR;
    uint16_t QTY;
    MODBUS_TYPE TYPE;
    MB_BYTE_ORDER ORDER; // 2. เปลี่ยนตรงนี้
    bool SIGN;
};

class MODBUS_RS485 {
public:
    MODBUS_RS485(HardwareSerial* PORT, uint8_t DE_PIN, uint8_t RE_PIN);
    void INIT(RS485_CONF CONF);
    void ADD_CH(MODBUS_CH CH);
    bool FETCH(uint8_t ID);
    void FETCH_ALL();
    uint32_t GET_DATA(uint8_t ID);

    // ฟังก์ชันสำหรับโหลด Config จาก SD Card
    bool loadConfig(SDResourceManager& sd, const char* path = "/CONFIG~1.JSO");

private:
    HardwareSerial* _SERIAL; // เปลี่ยนชื่อเพื่อหลบ Macro SERIAL ของ Arduino
    uint8_t _DE_PIN;
    uint8_t _RE_PIN;
    RS485_CONF CFG;
    MODBUS_CH CH_LIST[32];
    uint32_t CH_DATA[32];
    uint8_t CH_COUNT;

    uint16_t CALC_CRC16(uint8_t* BUF, uint8_t LEN); // เปลี่ยนชื่อหลบ Register CRC ของ STM32
    uint32_t APPLY_BYTE_ORDER(uint8_t* PAYLOAD, uint8_t LEN, MB_BYTE_ORDER ORDER);
    void TX_EN();
    void RX_EN();
};
#endif