#ifndef MYLIB_H
#define MYLIB_H
#pragma once
#include <Arduino.h>
#include <Wire.h>
//#include <U8g2lib.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <stdint.h>
#include <vector>
#include <Adafruit_SHTC3.h>
#include "device_config.h"
#include <EEPROM.h>
#include <RadioLib.h>

extern bool isLogging;

// ===================== LoRa P2P =====================
class LoRaP2P {
public:
  LoRaP2P();
  int16_t begin(float frequency);

  // send helpers
  int16_t send(const char* msg);
  int16_t sendBytes(const uint8_t* data, size_t len);

  // receive helpers
  String receive(uint32_t timeout = 1000);
  int16_t receiveBytes(uint8_t* buffer, size_t len, uint32_t timeout = 1000);

private:
  Module module;
  SX1278 radio;
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
  void sendNow(const char* payload);
  void loadConfig(const JsonObject& lora);
  void loadConfigFromStruct(const LoRaCfg& lora_cfg);
  void setMode(LoRaClassMode mode);  
  void classC();
  void classA();
  void end();
  bool available();
  const char* getDownlink();
  bool downlinkHandle();
  bool canSend();
  

private:
  LoRaClassMode currentMode = CLASS_C; 
  char downlinkText[256];
  bool hasNewDownlink = false;
};

// ===================== SD Resource Manager =====================

class SDResourceManager {
public:
    SDResourceManager(uint8_t mosi, uint8_t miso, uint8_t sck, uint8_t cs);

    bool begin();
    void listFiles(Stream& serial, const char* dirName = "/", int numTabs = 0);

    String readFile(const char* path);
    bool loadConfig(const char* path);

    String getLoRaKey() { return _loraKey; }
    int getSensorPin() { return _pin; }

    bool checkHardwares(const char* path, const char* hardwareName);

    const char* getConfig();

    bool writeLog(const char* message);
    void end();

private:
    uint8_t _mosi, _miso, _sck, _cs;

    String _loraKey;
    int _pin;
};

// ===================== Logger =====================

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
class OLED {
public:
    void begin();

    void clear();
    void update();
    void loadConfig(SDResourceManager& sd, const char* path = "/CONFIG~1.JSO");
    void loadConfigFromStruct(const HardwareCfg& hw);

    // ---------------- TEXT ----------------
    void setFont(const uint8_t* font);
    void print(const char* text, int x, int y); 
    void println(const char* text, int x, int y);   
    void drawStr(int x, int y, const char* text);
    void updateDisplay(const char* payload , const char* status);
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
    void loadConfig(const JsonObject& i2c);
    void loadConfig(const I2CConfig& i2c_cfg);
    void master_begin();
    const char* master_loop();
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
    uint32_t _max_resp_ms;
    static volatile uint8_t _currentRegister; 
    static uint8_t _registers[16]; 
    void resetInternalConfig();
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

  bool readHoldingRegisters(uint8_t slaveId, uint16_t startAddr, uint16_t quantity);

  bool update(uint8_t slaveId, uint16_t startAddr, uint16_t quantity);

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


// ================= ANALOG =================

class Analog420 {
public:
  Analog420();

  void begin();
  void loadConfigFromStruct(const HardwareCfg& hw);
  void loadConfigFromJson(const JsonObject& analog);
  float readCurrent();
  float readVoltage();
  float readRaw();
  float readScaled();    

private:
  Analog420Config _cfg;
};

// ===================== RS485 =====================
class SDResourceManager;

enum PARITY_OPT { MB_PARITY_NONE = 0, MB_PARITY_ODD, MB_PARITY_EVEN };
enum MODBUS_TYPE { MB_COIL = 1, MB_DISCRETE = 2, MB_HOLDING = 3, MB_INPUT = 4 };
enum MB_BYTE_ORDER { BO_AB = 0, BO_BA, BO_ABCD, BO_CDBA, BO_BADC, BO_DCBA };

struct RS485_CONF {
    bool ENABLE;
    uint32_t BAUD;
    uint8_t STOP;
    uint8_t DATA;        
    uint32_t INTERVAL;
    uint32_t MAX_RESP;
    uint8_t MAX_RETRY;
    uint8_t PARITY;      
};
#ifndef MAX_MODBUS_CHANNELS
#define MAX_MODBUS_CHANNELS 32
#endif

struct MODBUS_CH {
    uint8_t CH_ID;
    char NAME[16];
    uint8_t SLAVE;
    uint16_t ADDR;
    uint16_t QTY;
    MODBUS_TYPE TYPE;
    MB_BYTE_ORDER ORDER; 
    bool SIGN;
};

class MODBUS_RS485 {
public:
    MODBUS_RS485(HardwareSerial* PORT, uint8_t DE_PIN, uint8_t RE_PIN);
    void INIT(RS485_CONF CONF);
    void ADD_CH(MODBUS_CH CH);
    bool FETCH(uint8_t ID);
    void FETCH_ALL();
    int32_t GET_DATA(uint8_t ID);                   
    uint8_t GET_CH_COUNT();
    MODBUS_CH* GET_CH(uint8_t index);
    int32_t GET_DATA_BY_INDEX(uint8_t index);        
    bool loadConfigFromJson(const JsonObject& rs485);
    bool loadConfigFromStruct(const HardwareCfg& hw);

private:
    HardwareSerial* _SERIAL; 
    uint8_t _DE_PIN;
    uint8_t _RE_PIN;
    RS485_CONF CFG;
    MODBUS_CH CH_LIST[MAX_MODBUS_CHANNELS];   
    int32_t CH_DATA[MAX_MODBUS_CHANNELS];     
    uint8_t CH_COUNT;

    uint16_t CALC_CRC16(uint8_t* BUF, uint8_t LEN);
    int32_t APPLY_BYTE_ORDER(uint8_t* PAYLOAD, uint8_t LEN, MB_BYTE_ORDER ORDER, bool SIGN);
    void TX_EN();
    void RX_EN();
};


extern "C" {
  #include "stm32f1xx_hal.h"
}

typedef struct {
    bool ENABLE;
    GPIO_TypeDef* PORT;
    uint16_t PIN;
    bool INVERTED;
    GPIO_PinState DEFAULT_STATE;
    uint32_t MODE;
    uint32_t PULL;
    uint32_t SPEED;
    uint32_t STARTUP_DELAY;
} LS_CONF;

// ===================== Low Side Switch =====================

class LowSideSwitch {
private:
    LS_CONF conf;

public:
    LowSideSwitch();

    bool loadConfigFromJson(const JsonObject& sw);
    bool loadConfigFromStruct(const HardwareCfg& hw);
    void begin();

    void on();
    void off();
    void toggle();
};
class MySHTC3 {

private:

    TwoWire* _wire;

    uint8_t _sda;
    uint8_t _scl;

    Adafruit_SHTC3 _shtc3;

    float _temperature;
    float _humidity;

public:

    MySHTC3(TwoWire* wire, uint8_t sda, uint8_t scl);
  bool loadConfigFromStruct(const HardwareCfg& hw);

    bool begin();

    bool read();

    float getTemperature();

    float getHumidity();
};

#define CLI_COLOR_RESET   "\x1b[0m"
#define CLI_COLOR_RED     "\x1b[31m"
#define CLI_COLOR_GREEN   "\x1b[32m"
#define CLI_COLOR_YELLOW  "\x1b[33m"
#define CLI_COLOR_CYAN    "\x1b[36m"
#define CLI_COLOR_BOLD    "\x1b[1m"

// ===================== Serial CLI =====================

class SerialCLI {
public:
  void begin(Stream& serial, ConfigManager& cfgMgr, SDResourceManager* sd = nullptr, LowSideSwitch* ls = nullptr, LoRaWan* lorawan = nullptr);
  void update();
  void printPrompt();

private:
  Stream* _serial;
  ConfigManager* _cfgMgr;
  SDResourceManager* _sd;
  LowSideSwitch* _ls;
  LoRaWan* _lorawan;
  String _buffer;

  // Core CLI
  void processCommand(String cmdLine);
  void printHelp();
  void clearScreen();
  void printSuccess(const char* msg);
  void printError(const char* msg);
  void handleSetLogging(String key, String value);
  void handleSetComm(String key, String value);

  void handleSetCommand(String args);
  void handleSetDevice(String key, String value);
  void handleSetLoRa(String key, String value);
  void handleSetHardware(String key, String value);

  // --- Show Handlers ---
  void showConfig(String category);
  void showSDConfig();
  void showDeviceConfig();
  void showLoRaConfig();
  void showHardwareConfig();
  void showCommunicationConfig();
  void showLoggingConfig();

  // System commands
  void rebootSystem();
  void factoryReset();
};


#endif