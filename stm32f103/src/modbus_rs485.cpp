#include "mylib.h"


MODBUS_RS485::MODBUS_RS485(HardwareSerial* PORT, uint8_t DE_PIN, uint8_t RE_PIN) {
    _SERIAL = PORT;
    _DE_PIN = DE_PIN;
    _RE_PIN = RE_PIN;
    CH_COUNT = 0;
    for (int i = 0; i < 32; i++) CH_DATA[i] = 0;
}

void MODBUS_RS485::INIT(RS485_CONF CONF) {
    CFG = CONF;
    if (!CFG.ENABLE) return;

    pinMode(_DE_PIN, OUTPUT);
    pinMode(_RE_PIN, OUTPUT);
    RX_EN();

    uint32_t MODE = SERIAL_8N1;
    if (CFG.PARITY == MB_PARITY_NONE && CFG.STOP == 1) MODE = SERIAL_8N1;
    if (CFG.PARITY == MB_PARITY_NONE && CFG.STOP == 2) MODE = SERIAL_8N2;
    if (CFG.PARITY == MB_PARITY_EVEN && CFG.STOP == 1) MODE = SERIAL_8E1;
    if (CFG.PARITY == MB_PARITY_EVEN && CFG.STOP == 2) MODE = SERIAL_8E2;
    if (CFG.PARITY == MB_PARITY_ODD && CFG.STOP == 1) MODE = SERIAL_8O1;
    if (CFG.PARITY == MB_PARITY_ODD && CFG.STOP == 2) MODE = SERIAL_8O2;

    _SERIAL->begin(CFG.BAUD, MODE);
}

void MODBUS_RS485::ADD_CH(MODBUS_CH CH) {
    if (CH_COUNT < 32) {
        CH_LIST[CH_COUNT] = CH;
        CH_COUNT++;
    }
}

uint32_t MODBUS_RS485::GET_DATA(uint8_t ID) {
    for (uint8_t I = 0; I < CH_COUNT; I++) {
        if (CH_LIST[I].CH_ID == ID) return CH_DATA[I];
    }
    return 0;
}

bool MODBUS_RS485::FETCH(uint8_t ID) {
    if (!CFG.ENABLE) return false;

    MODBUS_CH TARGET;
    uint8_t TARGET_IDX = 0;
    bool FOUND = false;

    for (uint8_t I = 0; I < CH_COUNT; I++) {
        if (CH_LIST[I].CH_ID == ID) { TARGET = CH_LIST[I]; TARGET_IDX = I; FOUND = true; break; }
    }
    if (!FOUND) return false;

    uint8_t REQ[8];
    REQ[0] = TARGET.SLAVE; REQ[1] = TARGET.TYPE; REQ[2] = TARGET.ADDR >> 8;
    REQ[3] = TARGET.ADDR & 0xFF; REQ[4] = TARGET.QTY >> 8; REQ[5] = TARGET.QTY & 0xFF;

    uint16_t calc_crc = CALC_CRC16(REQ, 6);
    REQ[6] = calc_crc & 0xFF; REQ[7] = calc_crc >> 8;

    uint8_t RETRY = 0;
    bool SUCCESS = false;

    while (RETRY < CFG.MAX_RETRY && !SUCCESS) {
        while (_SERIAL->available()) _SERIAL->read();

        TX_EN();
        _SERIAL->write(REQ, 8);
        _SERIAL->flush();
        RX_EN();

        uint32_t START = millis();
        Serial1.print("Master: Waiting for Slave... ");
        
        while (millis() - START < CFG.MAX_RESP) {
            if (_SERIAL->available() >= 5) {
                delay(20);
                uint8_t RESP_LEN = _SERIAL->available();
                uint8_t RESP[64];
                _SERIAL->readBytes(RESP, RESP_LEN);

                Serial1.print("Got ");
                Serial1.print(RESP_LEN);
                Serial1.print(" bytes -> ");
                for(int i=0; i<RESP_LEN; i++) {
                    Serial1.print(RESP[i], HEX); Serial1.print(" ");
                }
                Serial1.println();

                uint16_t RECV_CRC = (uint16_t)RESP[RESP_LEN - 2] | ((uint16_t)RESP[RESP_LEN - 1] << 8);
                uint16_t COMP_CRC = CALC_CRC16(RESP, RESP_LEN - 2);

                if (RESP[0] == TARGET.SLAVE && RECV_CRC == COMP_CRC) {
                    Serial1.println("CRC MATCH! Data Valid.");
                    uint8_t BYTE_COUNT = RESP[2];
                    uint8_t* PAYLOAD = &RESP[3];
                    CH_DATA[TARGET_IDX] = APPLY_BYTE_ORDER(PAYLOAD, BYTE_COUNT, TARGET.ORDER);
                    SUCCESS = true;
                } else {
                    Serial1.println("FAILED: CRC or ID Mismatch!");
                }
                break;
            }
        }
        if (!SUCCESS) {
            Serial1.println("TIMEOUT: No response from Slave.");
            RETRY++;
        }
    }
    delay(CFG.INTERVAL);
    return SUCCESS;
}

void MODBUS_RS485::FETCH_ALL() {
    for (uint8_t I = 0; I < CH_COUNT; I++) {
        FETCH(CH_LIST[I].CH_ID);
    }
}

uint32_t MODBUS_RS485::APPLY_BYTE_ORDER(uint8_t* PAYLOAD, uint8_t LEN, MB_BYTE_ORDER ORDER) {
    uint32_t RESULT = 0;

    if (LEN == 2) { 
        switch (ORDER) {
            case BO_AB: RESULT = (PAYLOAD[0] << 8) | PAYLOAD[1]; break;
            case BO_BA: RESULT = (PAYLOAD[1] << 8) | PAYLOAD[0]; break;
            default:    RESULT = (PAYLOAD[0] << 8) | PAYLOAD[1]; break;
        }
    } 
    else if (LEN == 4) { 
        uint32_t b0 = PAYLOAD[0], b1 = PAYLOAD[1], b2 = PAYLOAD[2], b3 = PAYLOAD[3];
        switch (ORDER) {
            case BO_ABCD: RESULT = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3; break;
            case BO_CDBA: RESULT = (b2 << 24) | (b3 << 16) | (b0 << 8) | b1; break;
            case BO_BADC: RESULT = (b1 << 24) | (b0 << 16) | (b3 << 8) | b2; break;
            case BO_DCBA: RESULT = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0; break;
            default:      RESULT = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3; break;
        }
    }
    return RESULT;
}

uint16_t MODBUS_RS485::CALC_CRC16(uint8_t* BUF, uint8_t LEN) {
    uint16_t crc_val = 0xFFFF;
    for (uint8_t POS = 0; POS < LEN; POS++) {
        crc_val ^= (uint16_t)BUF[POS];
        for (uint8_t I = 8; I != 0; I--) {
            if ((crc_val & 0x0001) != 0) {
                crc_val >>= 1;
                crc_val ^= 0xA001;
            } else {
                crc_val >>= 1;
            }
        }
    }
    return crc_val;
}

void MODBUS_RS485::TX_EN() { 
    digitalWrite(_DE_PIN, HIGH); 
    digitalWrite(_RE_PIN, HIGH); 
}
void MODBUS_RS485::RX_EN() { 
    digitalWrite(_DE_PIN, LOW); 
    digitalWrite(_RE_PIN, LOW); 
}

uint8_t MODBUS_RS485::GET_CH_COUNT() {
    return CH_COUNT;
}

MODBUS_CH* MODBUS_RS485::GET_CH(uint8_t index) {
    if (index >= CH_COUNT) return nullptr;
    return &CH_LIST[index];
}
uint32_t MODBUS_RS485::GET_DATA_BY_INDEX(uint8_t index) {
    if (index >= CH_COUNT) return 0;
    return CH_DATA[index];
}
bool MODBUS_RS485::loadConfig(SDResourceManager& sd, const char* path) {
    String json = sd.readFile(path);
    if (json == "ERROR_OPEN" || json.length() == 0) {
        Serial1.print(F("MODBUS: failed to open config "));
        Serial1.println(path);
        return false; 
    }

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial1.print(F("MODBUS: JSON parse failed - "));
        Serial1.println(err.c_str());
        return false;
    }

    if (!doc["hardware"]["modbus_rs485"].is<JsonObject>()) {
        Serial1.println(F("MODBUS ERROR: หาคำว่า 'hardware' หรือ 'modbus_rs485' ในไฟล์ JSON ไม่เจอ!"));
        return false; 
    }
    
    JsonObject rs485 = doc["hardware"]["modbus_rs485"];

    RS485_CONF conf;
    conf.ENABLE    = rs485["enable"] | false;
    conf.BAUD      = rs485["baud_rate"] | 9600;
    conf.STOP      = rs485["stop_bit"] | 1;
    conf.DATA      = rs485["data_bit"] | 8;
    conf.INTERVAL  = rs485["interval_ms"] | 1000;
    conf.MAX_RESP  = rs485["max_resp_ms"] | 1000;
    conf.MAX_RETRY = rs485["max_retry"] | 3;

    String parityStr = rs485["parity"] | "NONE";
    if (parityStr == "ODD") conf.PARITY = MB_PARITY_ODD;
    else if (parityStr == "EVEN") conf.PARITY = MB_PARITY_EVEN;
    else conf.PARITY = MB_PARITY_NONE;

    this->INIT(conf);
    Serial1.println(F("MODBUS: Base Config Loaded from SD"));

    if (rs485["channels"].is<JsonArray>()) {
        JsonArray channels = rs485["channels"].as<JsonArray>();
        for (JsonVariant v : channels) {
            JsonObject ch = v.as<JsonObject>();
            MODBUS_CH m_ch;
            m_ch.CH_ID = ch["id"] | 1;
            strlcpy(m_ch.NAME, ch["name"] | "CH", sizeof(m_ch.NAME));
            m_ch.SLAVE = ch["slave_id"] | 1;
            m_ch.ADDR  = ch["address"] | 0;
            m_ch.QTY   = ch["quantity"] | 1;
            m_ch.SIGN  = ch["sign"] | false;

            String typeStr = ch["type"] | "HOLDING";
            if (typeStr == "COIL") m_ch.TYPE = MB_COIL;
            else if (typeStr == "DISCRETE") m_ch.TYPE = MB_DISCRETE;
            else if (typeStr == "INPUT") m_ch.TYPE = MB_INPUT;
            else m_ch.TYPE = MB_HOLDING;

            String orderStr = ch["byte_order"] | "AB";
            if (orderStr == "BA") m_ch.ORDER = BO_BA;
            else if (orderStr == "ABCD") m_ch.ORDER = BO_ABCD;
            else if (orderStr == "CDBA") m_ch.ORDER = BO_CDBA;
            else if (orderStr == "BADC") m_ch.ORDER = BO_BADC;
            else if (orderStr == "DCBA") m_ch.ORDER = BO_DCBA;
            else m_ch.ORDER = BO_AB;

            this->ADD_CH(m_ch);
        }
        Serial1.print(F("MODBUS: Loaded Channels = "));
        Serial1.println(this->CH_COUNT);
    }
    
    return true; 
}