#include "mylib.h"

SDResourceManager::SDResourceManager(uint8_t mosi, uint8_t miso, uint8_t sck, uint8_t cs) 
    : _mosi(mosi), _miso(miso), _sck(sck), _cs(cs) {}

bool SDResourceManager::begin() {
    SPI.setMOSI(_mosi);
    SPI.setMISO(_miso);
    SPI.setSCLK(_sck);
    return SD.begin(_cs);
}

// 1. ดูไฟล์ทั้งหมดใน SD Card
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

// 2. อ่านค่าไฟล์ใดๆ ตามชื่อที่ระบุ
String SDResourceManager::readFile(const char* path) {
    File file = SD.open(path);
    if (!file) return "ERROR_OPEN";
    String content = "";
    while (file.available()) content += (char)file.read();
    file.close();
    return content;
}

// 3. อ่าน Config และรองรับ Data แบบรังนก (Nested JSON)
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
        _loraKey = "KEY_NOT_FOUND"; // เพื่อให้รู้ว่าหาไม่เจอจริงๆ ไม่ใช่เปิดไฟล์ไม่ได้
    }

    file.close();
    return true;
}

// 4. บันทึก Log (.log)
bool SDResourceManager::writeLog(const char* message) {
    // ใช้ชื่อไฟล์ตายตัวหรือเปลี่ยนตามวันก็ได้
    File logFile = SD.open("/system.log", FILE_WRITE);
    if (logFile) {
        logFile.printf("[%lu] %s\n", millis(), message);
        logFile.close();
        return true;
    }
    return false;
}