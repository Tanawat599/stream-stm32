/**
 * @file modbus_rs485.cpp
 * @brief Modbus RTU Master implementation for RS485 communication.
 * 
 * This file provides a Modbus RTU master class for STM32F103.
 * It supports:
 * - Configurable baud rate, parity, stop bits, data bits.
 * - Reading holding registers (and optionally input registers, coils, discrete inputs).
 * - Automatic byte order conversion (AB, BA, ABCD, CDBA, BADC, DCBA).
 * - Signed/unsigned 16‑bit and 32‑bit values.
 * - Retry and timeout handling.
 * - Configuration loading from JSON (SD card) or struct (EEPROM).
 * - Multiple channels (each channel defines a register block to read).
 */

#include "mylib.h"

/* ========================= Constructor & Initialization ========================= */

/**
 * @brief Construct a new MODBUS_RS485 object.
 * @param PORT HardwareSerial port used for RS485 (e.g., &Serial2)
 * @param DE_PIN GPIO pin for Driver Enable (DE)
 * @param RE_PIN GPIO pin for Receiver Enable (RE)
 * 
 * Usually DE and RE are connected together and driven by a single pin.
 * The class will set DE/RE HIGH before transmitting and LOW after.
 */
MODBUS_RS485::MODBUS_RS485(HardwareSerial* PORT, uint8_t DE_PIN, uint8_t RE_PIN) {
    _SERIAL = PORT;
    _DE_PIN = DE_PIN;
    _RE_PIN = RE_PIN;
    CH_COUNT = 0;
    for (int i = 0; i < MAX_MODBUS_CHANNELS; i++) CH_DATA[i] = 0;
}

/**
 * @brief Initialize the Modbus master hardware and communication parameters.
 * @param CONF RS485_CONF structure containing baud, parity, data bits, stop bits,
 *            interval, max response time, max retries, and enable flag.
 * 
 * Configures the DE/RE pins as outputs, sets the serial port mode, and starts
 * the UART at the desired baud rate.
 */
void MODBUS_RS485::INIT(RS485_CONF CONF) {
    CFG = CONF;
    if (!CFG.ENABLE) return;

    pinMode(_DE_PIN, OUTPUT);
    pinMode(_RE_PIN, OUTPUT);
    RX_EN();

    uint32_t MODE;
    uint8_t data_bits = CFG.DATA;
    
    if (data_bits == 7) {
        Serial1.println(F("[MODBUS] Warning: 7 data bits not fully supported, using 8 data bits instead."));
        data_bits = 8;
    }
    
    if (data_bits == 8) {
        if (CFG.PARITY == MB_PARITY_NONE && CFG.STOP == 1) MODE = SERIAL_8N1;
        else if (CFG.PARITY == MB_PARITY_NONE && CFG.STOP == 2) MODE = SERIAL_8N2;
        else if (CFG.PARITY == MB_PARITY_EVEN && CFG.STOP == 1) MODE = SERIAL_8E1;
        else if (CFG.PARITY == MB_PARITY_EVEN && CFG.STOP == 2) MODE = SERIAL_8E2;
        else if (CFG.PARITY == MB_PARITY_ODD && CFG.STOP == 1) MODE = SERIAL_8O1;
        else if (CFG.PARITY == MB_PARITY_ODD && CFG.STOP == 2) MODE = SERIAL_8O2;
        else MODE = SERIAL_8N1; // fallback
    } else {

        Serial1.println(F("[MODBUS] Error: Unsupported data bits, using 8N1"));
        MODE = SERIAL_8N1;
    }

    _SERIAL->begin(CFG.BAUD, MODE);
}

/* ========================= Channel Management ========================= */

/**
 * @brief Add a channel definition to the master's channel list.
 * @param CH MODBUS_CH structure containing slave ID, register address, quantity,
 *           data type, byte order, and signedness.
 * 
 * Channels are stored internally. FETCH_ALL() will iterate over all added channels.
 */
void MODBUS_RS485::ADD_CH(MODBUS_CH CH) {
    if (CH_COUNT < MAX_MODBUS_CHANNELS) {
        CH_LIST[CH_COUNT] = CH;
        CH_COUNT++;
    }
}

/**
 * @brief Retrieve the last read value for a given channel ID.
 * @param ID Channel identifier (user‑assigned ID, not slave address)
 * @return int32_t The value as a 32‑bit signed integer (or 0 if ID not found).
 */
int32_t MODBUS_RS485::GET_DATA(uint8_t ID) {
    for (uint8_t I = 0; I < CH_COUNT; I++) {
        if (CH_LIST[I].CH_ID == ID) return CH_DATA[I];
    }
    return 0;
}

/**
 * @brief Read a single channel (slave/register) synchronously.
 * @param ID Channel identifier (must have been added via ADD_CH).
 * @return true if read succeeded, false after retries/timeout.
 * 
 * Internally builds the Modbus request frame (function code, address, quantity),
 * calculates CRC, transmits, waits for response, validates CRC, and stores the
 * result in CH_DATA[]. Uses the configured byte order and signedness.
 */
bool MODBUS_RS485::FETCH(uint8_t ID) {
    if (!CFG.ENABLE) return false;

    MODBUS_CH TARGET;
    uint8_t TARGET_IDX = 0;
    bool FOUND = false;

    for (uint8_t I = 0; I < CH_COUNT; I++) {
        if (CH_LIST[I].CH_ID == ID) {
            TARGET = CH_LIST[I];
            TARGET_IDX = I;
            FOUND = true;
            break;
        }
    }
    if (!FOUND) return false;

    uint8_t REQ[8];
    REQ[0] = TARGET.SLAVE;
    REQ[1] = TARGET.TYPE;
    REQ[2] = TARGET.ADDR >> 8;
    REQ[3] = TARGET.ADDR & 0xFF;
    REQ[4] = TARGET.QTY >> 8;
    REQ[5] = TARGET.QTY & 0xFF;

    uint16_t calc_crc = CALC_CRC16(REQ, 6);
    REQ[6] = calc_crc & 0xFF;
    REQ[7] = calc_crc >> 8;

    uint8_t RETRY = 0;
    bool SUCCESS = false;
    uint8_t EXPECTED_LEN = 3 + (TARGET.QTY * 2) + 2;

    while (RETRY < CFG.MAX_RETRY && !SUCCESS) {
        while (_SERIAL->available()) _SERIAL->read();

        TX_EN();
        _SERIAL->write(REQ, 8);
        _SERIAL->flush();
        delay(2);
        RX_EN();

        uint32_t START = millis();

        while (millis() - START < CFG.MAX_RESP) {
            if (_SERIAL->available() >= EXPECTED_LEN) {
                uint8_t RESP[64];
                _SERIAL->readBytes(RESP, EXPECTED_LEN);

                uint16_t RECV_CRC = (uint16_t)RESP[EXPECTED_LEN - 2] | ((uint16_t)RESP[EXPECTED_LEN - 1] << 8);
                uint16_t COMP_CRC = CALC_CRC16(RESP, EXPECTED_LEN - 2);

                if (RESP[0] == TARGET.SLAVE && RECV_CRC == COMP_CRC) {
                    uint8_t BYTE_COUNT = RESP[2];
                    uint8_t* PAYLOAD = &RESP[3];
                    CH_DATA[TARGET_IDX] = APPLY_BYTE_ORDER(PAYLOAD, BYTE_COUNT, TARGET.ORDER, TARGET.SIGN);
                    SUCCESS = true;
                } else {
                    Serial1.printf("[MODBUS] 0x%02X: CRC/ID mismatch (retry %d/%d)\n",
                                   TARGET.SLAVE, RETRY + 1, CFG.MAX_RETRY);
                }
                break; 
            }
        }

        if (!SUCCESS) {
            // Timeout
            Serial1.printf("[MODBUS] 0x%02X: timeout %lu ms (retry %d/%d)\n",
                           TARGET.SLAVE, millis() - START, RETRY + 1, CFG.MAX_RETRY);
            RETRY++;
            if (RETRY < CFG.MAX_RETRY) delay(200);
        }
    }

    if (!SUCCESS) {
        Serial1.printf("[MODBUS] FAILED to read 0x%02X after %d retries\n",
                       TARGET.SLAVE, CFG.MAX_RETRY);
    }

    delay(CFG.INTERVAL);
    return SUCCESS;
}

void MODBUS_RS485::FETCH_ALL() {
    for (uint8_t I = 0; I < CH_COUNT; I++) {
        FETCH(CH_LIST[I].CH_ID);
    }
}

/* ========================= Helper Functions ========================= */

/**
 * @brief Convert raw Modbus register bytes to a 32‑bit signed integer.
 * @param PAYLOAD Pointer to the data bytes (starting after byte count).
 * @param LEN Number of bytes (2 for 16‑bit, 4 for 32‑bit).
 * @param ORDER Byte order (endianness) configuration.
 * @param SIGN True if the value should be interpreted as signed.
 * @return int32_t Converted value.
 * 
 * Supported orders:
 * - AB: high byte first (big‑endian, 16‑bit)
 * - BA: low byte first (little‑endian, 16‑bit)
 * - ABCD: big‑endian 32‑bit (most significant byte first)
 * - CDBA: swap words then bytes (useful for some Modbus devices)
 * - BADC: swap bytes within each word (word swap)
 * - DCBA: fully reversed (little‑endian 32‑bit)
 */
int32_t MODBUS_RS485::APPLY_BYTE_ORDER(uint8_t* PAYLOAD, uint8_t LEN, MB_BYTE_ORDER ORDER, bool SIGN) {
    uint32_t raw = 0;

    if (LEN == 2) {
        switch (ORDER) {
            case BO_AB: raw = (PAYLOAD[0] << 8) | PAYLOAD[1]; break;
            case BO_BA: raw = (PAYLOAD[1] << 8) | PAYLOAD[0]; break;
            default:    raw = (PAYLOAD[0] << 8) | PAYLOAD[1]; break;
        }
        if (SIGN) {
            int16_t val = (int16_t)raw;
            return (int32_t)val;
        }
        return (int32_t)raw;
    }
    else if (LEN == 4) {
        uint32_t b0 = PAYLOAD[0], b1 = PAYLOAD[1], b2 = PAYLOAD[2], b3 = PAYLOAD[3];
        switch (ORDER) {
            case BO_ABCD: raw = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3; break;
            case BO_CDBA: raw = (b2 << 24) | (b3 << 16) | (b0 << 8) | b1; break;
            case BO_BADC: raw = (b1 << 24) | (b0 << 16) | (b3 << 8) | b2; break;
            case BO_DCBA: raw = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0; break;
            default:      raw = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3; break;
        }
        if (SIGN) {
            return (int32_t)raw;
        }
        return (int32_t)raw;
    }
    return 0;
}

/**
 * @brief Compute Modbus RTU CRC‑16 (CRC‑16‑IBM, polynomial 0xA001).
 * @param BUF Pointer to data bytes.
 * @param LEN Number of bytes.
 * @return uint16_t Calculated CRC value (low byte first in the frame).
 */
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
/**
 * @brief Enable transmitter mode (DE=HIGH, RE=HIGH).
 * 
 * For MAX485, connecting DE and RE together works:
 * - HIGH = transmit
 * - LOW  = receive
 */
void MODBUS_RS485::TX_EN() {
    digitalWrite(_DE_PIN, HIGH);
    digitalWrite(_RE_PIN, HIGH);
}
void MODBUS_RS485::RX_EN() {
    digitalWrite(_DE_PIN, LOW);
    digitalWrite(_RE_PIN, LOW);
}

/* ========================= Getters ========================= */

/**
 * @brief Return the number of configured channels.
 */
uint8_t MODBUS_RS485::GET_CH_COUNT() {
    return CH_COUNT;
}

/**
 * @brief Get a pointer to the channel configuration by index.
 * @param index 0‑based index (0 … CH_COUNT-1)
 * @return MODBUS_CH* Pointer to the channel, or nullptr if out of range.
 */
MODBUS_CH* MODBUS_RS485::GET_CH(uint8_t index) {
    if (index >= CH_COUNT) return nullptr;
    return &CH_LIST[index];
}

/**
 * @brief Get the last read value for a channel by index.
 * @param index 0‑based index.
 * @return int32_t Stored value.
 */
int32_t MODBUS_RS485::GET_DATA_BY_INDEX(uint8_t index) {
    if (index >= CH_COUNT) return 0;
    return CH_DATA[index];
}

bool MODBUS_RS485::loadConfigFromJson(const JsonObject& rs485) {
    RS485_CONF conf;
    conf.ENABLE    = rs485["enable"] | false;
    conf.BAUD      = rs485["baud_rate"] | 9600;
    conf.STOP      = rs485["stop_bit"] | 1;
    conf.DATA      = rs485["data_bit"] | 8;
    conf.INTERVAL  = rs485["interval_ms"] | 1000;
    conf.MAX_RESP  = rs485["max_resp_ms"] | 1000;
    conf.MAX_RETRY = rs485["max_retry"] | 3;

    const char* parityStr = rs485["parity"] | "NONE";
    if (strcmp(parityStr, "ODD") == 0) conf.PARITY = MB_PARITY_ODD;
    else if (strcmp(parityStr, "EVEN") == 0) conf.PARITY = MB_PARITY_EVEN;
    else conf.PARITY = MB_PARITY_NONE;

    this->INIT(conf);

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

            const char* typeStr = ch["type"] | "HOLDING";
            if (strcmp(typeStr, "COIL") == 0) m_ch.TYPE = MB_COIL;
            else if (strcmp(typeStr, "DISCRETE") == 0) m_ch.TYPE = MB_DISCRETE;
            else if (strcmp(typeStr, "INPUT") == 0) m_ch.TYPE = MB_INPUT;
            else m_ch.TYPE = MB_HOLDING;

            const char* orderStr = ch["byte_order"] | "AB";
            if (strcmp(orderStr, "BA") == 0) m_ch.ORDER = BO_BA;
            else if (strcmp(orderStr, "ABCD") == 0) m_ch.ORDER = BO_ABCD;
            else if (strcmp(orderStr, "CDBA") == 0) m_ch.ORDER = BO_CDBA;
            else if (strcmp(orderStr, "BADC") == 0) m_ch.ORDER = BO_BADC;
            else if (strcmp(orderStr, "DCBA") == 0) m_ch.ORDER = BO_DCBA;
            else m_ch.ORDER = BO_AB;

            if (this->CH_COUNT >= MAX_MODBUS_CHANNELS) {
                Serial1.println(F("[MODBUS] Error: Max channels reached, ignoring extra"));
                continue;
            }
            this->ADD_CH(m_ch);
        }
        Serial1.print(F("[MODBUS] Enabled: "));
        Serial1.print(conf.ENABLE ? "1" : "0");
        Serial1.print(F(", Baud: "));
        Serial1.print(conf.BAUD);
        Serial1.print(F(", Parity: "));
        if (conf.PARITY == MB_PARITY_ODD) Serial1.print("ODD");
        else if (conf.PARITY == MB_PARITY_EVEN) Serial1.print("EVEN");
        else Serial1.print("NONE");
        Serial1.print(F(", Interval: "));
        Serial1.print(conf.INTERVAL);
        Serial1.print(F("ms, MaxResp: "));
        Serial1.print(conf.MAX_RESP);
        Serial1.print(F("ms, Retry: "));
        Serial1.println(conf.MAX_RETRY);
        Serial1.print(F("[MODBUS] Loaded Channels = "));
        Serial1.println(this->CH_COUNT);
        Serial1.println();
    }
    return true;
}

bool MODBUS_RS485::loadConfigFromStruct(const HardwareCfg& hw) {
    RS485_CONF conf;
    conf.ENABLE   = hw.modbus_rs485.enable;
    conf.BAUD     = hw.modbus_rs485.baud_rate;
    conf.STOP     = hw.modbus_rs485.stop_bit;
    conf.DATA     = hw.modbus_rs485.data_bit;
    conf.INTERVAL = hw.modbus_rs485.interval_ms;
    conf.MAX_RESP = hw.modbus_rs485.max_resp_ms;
    conf.MAX_RETRY= hw.modbus_rs485.max_retry;

    const char* parityStr = hw.modbus_rs485.parity;
    if (strcmp(parityStr, "ODD") == 0) conf.PARITY = MB_PARITY_ODD;
    else if (strcmp(parityStr, "EVEN") == 0) conf.PARITY = MB_PARITY_EVEN;
    else conf.PARITY = MB_PARITY_NONE;

    this->INIT(conf);

    for (int i = 0; i < MAX_MODBUS_CHANNELS; i++) {
        const ModbusChannelCfg& ch = hw.modbus_rs485.channels[i];
        if (ch.id == 0) continue;

        MODBUS_CH m_ch;
        m_ch.CH_ID = ch.id;
        strlcpy(m_ch.NAME, ch.name, sizeof(m_ch.NAME));
        m_ch.SLAVE = ch.slave_id;
        m_ch.ADDR  = ch.address;
        m_ch.QTY   = ch.quantity;
        m_ch.SIGN  = ch.sign;

        const char* typeStr = ch.type;
        if (strcmp(typeStr, "COIL") == 0) m_ch.TYPE = MB_COIL;
        else if (strcmp(typeStr, "INPUT") == 0) m_ch.TYPE = MB_INPUT;
        else m_ch.TYPE = MB_HOLDING;

        const char* orderStr = ch.byte_order;
        if (strcmp(orderStr, "BA") == 0) m_ch.ORDER = BO_BA;
        else if (strcmp(orderStr, "ABCD") == 0) m_ch.ORDER = BO_ABCD;
        else if (strcmp(orderStr, "CDBA") == 0) m_ch.ORDER = BO_CDBA;
        else if (strcmp(orderStr, "BADC") == 0) m_ch.ORDER = BO_BADC;
        else if (strcmp(orderStr, "DCBA") == 0) m_ch.ORDER = BO_DCBA;
        else m_ch.ORDER = BO_AB;

        if (this->CH_COUNT >= MAX_MODBUS_CHANNELS) {
            Serial1.println(F("[MODBUS] Error: Max channels reached, ignoring extra"));
            continue;
        }
        this->ADD_CH(m_ch);
    }
    Serial1.print(F("[MODBUS] Enabled: "));
    Serial1.print(conf.ENABLE ? "1" : "0");
    Serial1.print(F(", Baud: "));
    Serial1.print(conf.BAUD);
    Serial1.print(F(", Parity: "));
    if (conf.PARITY == MB_PARITY_ODD) Serial1.print("ODD");
    else if (conf.PARITY == MB_PARITY_EVEN) Serial1.print("EVEN");
    else Serial1.print("NONE");
    Serial1.print(F(", Interval: "));
    Serial1.print(conf.INTERVAL);
    Serial1.print(F("ms, MaxResp: "));
    Serial1.print(conf.MAX_RESP);
    Serial1.print(F("ms, Retry: "));
    Serial1.println(conf.MAX_RETRY);
    Serial1.print(F("[MODBUS] Loaded Channels = "));
    Serial1.println(this->CH_COUNT);
    Serial1.println();

    return true;
}