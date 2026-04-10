#include "mylib.h"

SDResourceManager::SDResourceManager(uint8_t mosi, uint8_t miso, uint8_t sck, uint8_t cs) 
    : _mosi(mosi), _miso(miso), _sck(sck), _cs(cs) {}

bool SDResourceManager::begin() {
    SPI.setMOSI(_mosi);
    SPI.setMISO(_miso);
    SPI.setSCLK(_sck);
    return SD.begin(_cs);
}

void SDResourceManager::listFiles(Stream& serial, const char* dirName, int numTabs) {
    File dir = SD.open(dirName);
    if (!dir) return;
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;
        for (uint8_t i = 0; i < numTabs; i++) serial.print("  ");
        serial.print(entry.name());
        if (entry.isDirectory()) {
            serial.println("/");
            listFiles(serial, entry.name(), numTabs + 1);
        } else {
            serial.printf("\t\t%d bytes\n", entry.size());
        }
        entry.close();
    }
    dir.close();
}

String SDResourceManager::readFile(const char* path) {
    File file = SD.open(path);
    if (!file) return "ERROR_OPEN";
    String content = "";
    while (file.available()) content += (char)file.read();
    file.close();
    return content;
}

bool SDResourceManager::loadConfig(const char* path) {
    File file = SD.open(path);
    if (!file) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        file.close();
        return false;
    }

    // วิธีดึงค่าแบบปลอดภัย (เช็คทีละชั้น)
    if (doc["lora"]["keys"].containsKey("app_key")) {
        _loraKey = doc["lora"]["keys"]["app_key"].as<String>();
    } else {
        _loraKey = "KEY_NOT_FOUND"; 
    }

    file.close();
    return true;
}

bool SDResourceManager::writeLog(const char* message) {
    File logFile = SD.open("/system.log", FILE_WRITE);
    if (logFile) {
        logFile.printf("[%lu] %s\n", millis(), message);
        logFile.close();
        return true;
    }
    return false;
}

Logger::Logger(SDResourceManager* sd) {
    _sd = sd;
}

// ===== Key-Value Log =====
void Logger::logKV(const char* type, int count, ...) {
    char buffer[256];
    int offset = 0;

    offset += sprintf(buffer + offset, "[%lu] [%s] ", millis(), type);

    va_list args;
    va_start(args, count);

    for (int i = 0; i < count; i++) {
        const char* key = va_arg(args, const char*);
        float value = va_arg(args, double); // float → double
        const char* unit = va_arg(args, const char*);

        offset += sprintf(buffer + offset, "%s=%.2f%s ", key, value, unit);
    }

    va_end(args);

    _sd->writeLog(buffer);
}

// ===== Message Log =====
void Logger::logMsg(const char* type, const char* message) {
    char buffer[256];
    sprintf(buffer, "[%lu] [%s] %s", millis(), type, message);
    _sd->writeLog(buffer);
}

// ===== Mixed Log =====
void Logger::logMixed(const char* type, const char* message, int count, ...) {
    char buffer[256];
    int offset = 0;

    offset += sprintf(buffer + offset, "[%lu] [%s] %s ", millis(), type, message);

    va_list args;
    va_start(args, count);

    for (int i = 0; i < count; i++) {
        const char* key = va_arg(args, const char*);
        float value = va_arg(args, double);
        const char* unit = va_arg(args, const char*);

        offset += sprintf(buffer + offset, "%s=%.2f%s ", key, value, unit);
    }

    va_end(args);

    _sd->writeLog(buffer);
}