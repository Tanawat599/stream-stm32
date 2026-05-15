/**
 * @file serial_cli.cpp
 * @brief Serial Command Line Interface (CLI) for configuring the LoRa Node on STM32.
 * 
 * This file implements a VT100 compatible serial CLI that allows the user to:
 * - View and modify device, hardware, LoRaWAN, logging, and communication settings.
 * - Save configuration to EEPROM.
 * - Reboot, factory reset, and control hardware (LED, LS switch) interactively.
 * - Read/write SD card files (if available).
 * - Send manual LoRaWAN uplinks.
 * 
 * The CLI is active when `isLogging` is false (entered by pressing any key during normal operation).
 * Commands are processed in `processCommand()` and dispatched to appropriate handlers.
 */

#include "mylib.h"
#include "device_config.h"

// ======================== Initialization ========================
/**
 * @brief Initialize the CLI with references to system components.
 * @param serial Reference to the Stream (usually Serial1) for input/output.
 * @param cfgMgr Reference to the ConfigManager for accessing device configuration.
 * @param sd Pointer to SDResourceManager (may be null if SD not available).
 * @param ls Pointer to LowSideSwitch (may be null).
 * @param lorawan Pointer to LoRaWan object (may be null).
 */

void SerialCLI::begin(Stream& serial, ConfigManager& cfgMgr, SDResourceManager* sd, LowSideSwitch* ls, LoRaWan* lorawan) {
    _serial = &serial;
    _cfgMgr = &cfgMgr;
    _sd = sd;
    _ls = ls;
    _lorawan = lorawan;
    _buffer = "";
    _buffer.reserve(64);

    clearScreen();

    _serial->println(CLI_COLOR_CYAN "========================================" CLI_COLOR_RESET);
    _serial->println(CLI_COLOR_BOLD "             LORA NODE CLI              " CLI_COLOR_RESET);
    _serial->println(CLI_COLOR_CYAN "========================================" CLI_COLOR_RESET);
    _serial->println("Type " CLI_COLOR_YELLOW "'help'" CLI_COLOR_RESET " to view available commands.\n");

    printPrompt();
}

// ======================== Main update loop ========================
/**
 * @brief Polls the serial port, handles character input, line editing, and command execution.
 * 
 * This method must be called frequently (e.g., in loop() when CLI mode is active).
 * It reads characters, echoes them back, handles backspace, and when Enter is pressed,
 * it passes the command line to `processCommand()`.
 */
void SerialCLI::update() {
    while (_serial->available()) {
        char c = _serial->read();

        if (c == '\n' || c == '\r') {
            if (_buffer.length() > 0) {
                _serial->println();
                processCommand(_buffer);
                _buffer = "";
            } else {
                _serial->println();
            }
            printPrompt();
        }
        else if (c == '\b' || c == 127) {
            if (_buffer.length() > 0) {
                _buffer.remove(_buffer.length() - 1);
                _serial->print("\b \b"); 
            }
        }
        else if (c >= 32 && c <= 126) {
            _buffer += c;
            _serial->print(c); 
        }
    }
}

/**
 * @brief Clear the terminal screen using ANSI escape codes.
 */
void SerialCLI::clearScreen() {
    _serial->print("\x1b[2J"); // Clear screen
    _serial->print("\x1b[H");  // Move cursor to top-left
}

// ======================== Command Dispatcher ========================
/**
 * @brief Parse and execute a command line.
 * @param cmdLine The full command string (e.g., "set device.name MyNode").
 * 
 * Recognized commands: help, show, exit/resume, set, save, factory-reset, reboot,
 * sd.list, sd.read, sd.config, uplink, toggle, i2c scan.
 */
void SerialCLI::processCommand(String cmdLine) {
    cmdLine.trim();
    if (cmdLine.length() == 0) return;

    int spaceIndex = cmdLine.indexOf(' ');
    String cmd = cmdLine;
    String args = "";

    if (spaceIndex != -1) {
        cmd = cmdLine.substring(0, spaceIndex);
        args = cmdLine.substring(spaceIndex + 1);
        args.trim();
    }
    cmd.toLowerCase();

    if (cmd == "help" || cmd == "h" || cmd == "?") {
        printHelp();
    } 
    else if (cmd == "show") {
        showConfig(args); 
    } 
    else if (cmd == "exit" || cmd == "resume") {
        isLogging = true;
        printSuccess("Resuming logging");
    }
    else if (cmd == "set") {
        if (args.length() == 0) printError("Usage: set <category.key> <value>");
        else handleSetCommand(args);
    } 
    else if (cmd == "save") {
        _cfgMgr->save();
        printSuccess("Config saved to EEPROM.");
        _serial->println("Reboot now to apply changes? (y/n)");
        
        while (_serial->available()) _serial->read();
        
        uint32_t start = millis();
        while (millis() - start < 5000) {
            if (_serial->available()) {
                char c = _serial->read();
                if (c == 'y' || c == 'Y') {
                    _serial->println("\nRebooting...");
                    delay(100);
                    rebootSystem();
                    return; 
                } else if (c == 'n' || c == 'N') {
                    _serial->println("\nReboot cancelled.");
                    return;
                }
            }
        }
        _serial->println("\nNo response, reboot cancelled.");
    }
    else if (cmd == "factory-reset") {
        factoryReset();
    } 
    else if (cmd == "reboot") {
        rebootSystem();
    } 
    // else if (cmd == "i2c" && args == "scan") {
    //     _serial->println("Scanning I2C bus...");
    //     byte error, address;
    //     int nDevices = 0;
    //     for(address = 1; address < 127; address++ ) {
    //         Wire.beginTransmission(address);
    //         error = Wire.endTransmission();
    //         if (error == 0) {
    //             _serial->print("I2C device found at address 0x");
    //             if (address < 16) _serial->print("0");
    //             _serial->println(address, HEX);
    //             nDevices++;
    //         }
    //     }
    //     if (nDevices == 0) _serial->println("No I2C devices found\n");
    //     else _serial->println("done\n");
    // }
    else {
        printError("Unknown command.");
    }
}

// ======================== Set Command Handling ========================
/**
 * @brief Handle the 'set' command: dispatch to category-specific handler.
 * @param args The arguments after "set", e.g., "device.name MyNode".
 */
void SerialCLI::handleSetCommand(String args) {
    int spaceIndex = args.indexOf(' ');
    if (spaceIndex == -1) {
        printError("Invalid format. Usage: set <category.key> <value>");
        return;
    }

    String fullKey = args.substring(0, spaceIndex);
    String value = args.substring(spaceIndex + 1);
    fullKey.toLowerCase();
    value.trim();


    int dotIndex = fullKey.indexOf('.');
    if (dotIndex == -1) {
        printError("Keys must use dot notation (e.g., device.name, lora.enabled)");
        return;
    }

    String category = fullKey.substring(0, dotIndex);
    String subKey = fullKey.substring(dotIndex + 1);

    if (category == "device") {
        handleSetDevice(subKey, value);
    } 
    else if (category == "hardware" || category == "hw") {
        handleSetHardware(subKey, value);
    } 
    else if (category == "logging" || category == "log") {
        handleSetLogging(subKey, value);
    } 
    else if (category == "communication" || category == "comm") {
        handleSetComm(subKey, value);
    } 
    else if (category == "lora") {
        handleSetLoRa(subKey, value);
    } 
    else {
        printError("Unknown category. Use 'device', 'hw', 'log', 'comm', or 'lora'.");
    }
}


void SerialCLI::handleSetDevice(String key, String value) {
    DeviceConfig& cfg = _cfgMgr->get();
    
    if (key == "name") {
        strncpy(cfg.device.name, value.c_str(), sizeof(cfg.device.name) - 1);
        printSuccess("Updated device.name");
    } else if (key == "type") {
        strncpy(cfg.device.type, value.c_str(), sizeof(cfg.device.type) - 1);
        printSuccess("Updated device.type");
    } else if (key == "id") {
        strncpy(cfg.device.id, value.c_str(), sizeof(cfg.device.id) - 1);
        printSuccess("Updated device.id");
    } else if (key == "uid") {
        strncpy(cfg.device.uid, value.c_str(), sizeof(cfg.device.uid) - 1);
        printSuccess("Updated device.uid");
    } else if (key == "use_sd_config") {
        cfg.device.use_sd_config = (value == "1" || value.equalsIgnoreCase("true"));
        printSuccess("Updated device.use_sd_config");
    }
     else {
        printError("Unknown key in 'device'. Try: name, type, id, uid");
    }
}

void SerialCLI::handleSetHardware(String key, String value) {
    DeviceConfig& cfg = _cfgMgr->get();
    
// ---------- I2C Configuration (base + devices/channels) ----------
    if (key.startsWith("i2c.")) {
        if (key.indexOf("device[") != -1) {
            int devOpen = key.indexOf('[');
            int devClose = key.indexOf(']', devOpen);
            if (devOpen == -1 || devClose == -1) {
                printError("Invalid I2C device format. Use i2c.device[<idx>].<field>");
                return;
            }
            String devIdxStr = key.substring(devOpen + 1, devClose);
            int devIdx = devIdxStr.toInt();
            if (devIdx < 0 || devIdx >= MAX_I2C_DEVICES) {
                printError("Device index out of range");
                return;
            }

            String afterDev = key.substring(devClose + 1);
            if (afterDev.length() == 0) {
                printError("Missing field after device index");
                return;
            }
            if (afterDev[0] == '.') afterDev = afterDev.substring(1);

            if (afterDev.startsWith("channel[")) {
                int chOpen = afterDev.indexOf('[');
                int chClose = afterDev.indexOf(']', chOpen);
                if (chOpen == -1 || chClose == -1) {
                    printError("Invalid channel format. Use i2c.device[<idx>].channel[<ch_idx>].<field>");
                    return;
                }
                String chIdxStr = afterDev.substring(chOpen + 1, chClose);
                int chIdx = chIdxStr.toInt();
                if (chIdx < 0 || chIdx >= MAX_I2C_CHANNELS) {
                    printError("Channel index out of range");
                    return;
                }
                String field = afterDev.substring(chClose + 1);
                if (field.length() == 0 || field[0] != '.') {
                    printError("Missing field name after channel index");
                    return;
                }
                field = field.substring(1);

                I2CChannelCfg& ch = cfg.hardware.i2c.devices[devIdx].channels[chIdx];

                if (field == "name") {
                    strncpy(ch.name, value.c_str(), sizeof(ch.name) - 1);
                    ch.name[sizeof(ch.name) - 1] = '\0';
                    String msg = "i2c.device[" + devIdxStr + "].channel[" + chIdxStr + "].name = " + value;
                    printSuccess(msg.c_str());
                }
                else if (field == "reg_addr" || field == "reg") {
                    int addr = strtol(value.c_str(), NULL, 0);
                    if (addr >= 0 && addr <= 0xFF) {
                        ch.reg_addr = (uint8_t)addr;
                        String msg = "i2c.device[" + devIdxStr + "].channel[" + chIdxStr + "].reg_addr = 0x" + String(addr, HEX);
                        printSuccess(msg.c_str());
                    } else {
                        printError("Register address must be 0x00-0xFF");
                    }
                }
                else if (field == "length") {
                    int len = value.toInt();
                    if (len >= 1 && len <= 4) {
                        ch.length = (uint8_t)len;
                        String msg = "i2c.device[" + devIdxStr + "].channel[" + chIdxStr + "].length = " + value;
                        printSuccess(msg.c_str());
                    } else {
                        printError("Length must be 1-4 bytes");
                    }
                }
                else if (field == "byte_order") {
                    String upVal = value;
                    upVal.toUpperCase();
                    if (upVal == "AB" || upVal == "BA" || upVal == "ABCD" || upVal == "DCBA") {
                        strncpy(ch.byte_order, upVal.c_str(), sizeof(ch.byte_order) - 1);
                        ch.byte_order[sizeof(ch.byte_order) - 1] = '\0';
                        String msg = "i2c.device[" + devIdxStr + "].channel[" + chIdxStr + "].byte_order = " + upVal;
                        printSuccess(msg.c_str());
                    } else {
                        printError("byte_order must be AB, BA, ABCD, or DCBA");
                    }
                }
                else if (field == "scale") {
                    ch.scale = value.toFloat();
                    char buf[16];
                    dtostrf(ch.scale, 6, 4, buf);
                    String msg = "i2c.device[" + devIdxStr + "].channel[" + chIdxStr + "].scale = " + String(buf);
                    printSuccess(msg.c_str());
                }
                else {
                    String err = "Unknown I2C channel field: " + field;
                    printError(err.c_str());
                }
            }
            else {
                I2CDeviceCfg& dev = cfg.hardware.i2c.devices[devIdx];
                if (afterDev == "name") {
                    strncpy(dev.name, value.c_str(), sizeof(dev.name) - 1);
                    dev.name[sizeof(dev.name) - 1] = '\0';
                    String msg = "i2c.device[" + devIdxStr + "].name = " + value;
                    printSuccess(msg.c_str());
                }
                else if (afterDev == "address" || afterDev == "addr") {
                    int addr = strtol(value.c_str(), NULL, 0);
                    if (addr >= 0 && addr <= 0x7F) {
                        dev.address = (uint8_t)addr;
                        String msg = "i2c.device[" + devIdxStr + "].address = 0x" + String(addr, HEX);
                        printSuccess(msg.c_str());
                    } else {
                        printError("I2C address must be 0x00-0x7F");
                    }
                }
                else if (afterDev == "id") {
                    int id = value.toInt();
                    if (id >= 0 && id <= 255) {
                        dev.id = (uint8_t)id;
                        String msg = "i2c.device[" + devIdxStr + "].id = " + value;
                        printSuccess(msg.c_str());
                    } else {
                        printError("Device id must be 0-255");
                    }
                }
                else {
                    String err = "Unknown I2C device field: " + afterDev;
                    printError(err.c_str());
                }
            }
            _cfgMgr->save();
            return;
        }
        else {
            String field = key.substring(4);
            bool success = true;

            if (field == "enable") {
                cfg.hardware.i2c.enable = (value == "1" || value.equalsIgnoreCase("true"));
            }
            else if (field == "frequency" || field == "freq") {
                cfg.hardware.i2c.frequency = value.toInt();
            }
            else if (field == "interval_ms" || field == "interval") {
                cfg.hardware.i2c.interval_ms = value.toInt();
            }
            else if (field == "master_address") {
                int addr = strtol(value.c_str(), NULL, 0);
                if (addr >= 0 && addr <= 127) cfg.hardware.i2c.master_address = (uint8_t)addr;
                else success = false;
            }
            else if (field == "max_resp_ms") {
                cfg.hardware.i2c.max_resp_ms = value.toInt();
            }
            else if (field == "max_retry") {
                int retry = value.toInt();
                if (retry >= 0 && retry <= 255) cfg.hardware.i2c.max_retry = (uint8_t)retry;
                else success = false;
            }
            else {
                printError("Unknown key in 'i2c'. Use: enable, frequency, interval_ms, master_address, max_resp_ms, max_retry");
                return;
            }

            if (success) {
                _cfgMgr->save();
                String msg = "i2c." + field + " = " + value;
                printSuccess(msg.c_str());
            } else {
                printError("Invalid value");
            }
            return;
        }
    }
    else if (key == "modbus.enable") {
        cfg.hardware.modbus_rs485.enable = (value == "1" || value == "true");
        printSuccess("Updated hw.modbus.enable");
    }
    else if (key == "modbus.baud_rate") {
        cfg.hardware.modbus_rs485.baud_rate = value.toInt();
        printSuccess("Updated hw.modbus.baud_rate");
    }
    else if (key == "modbus.data_bit") {
        cfg.hardware.modbus_rs485.data_bit = value.toInt();
        printSuccess("Updated hw.modbus.data_bit");
    }
    else if (key == "modbus.stop_bit") {
        cfg.hardware.modbus_rs485.stop_bit = value.toInt();
        printSuccess("Updated hw.modbus.stop_bit");
    }
    else if (key == "modbus.parity") {
        strncpy(cfg.hardware.modbus_rs485.parity, value.c_str(),
                sizeof(cfg.hardware.modbus_rs485.parity) - 1);
        cfg.hardware.modbus_rs485.parity[sizeof(cfg.hardware.modbus_rs485.parity) - 1] = '\0';
        printSuccess("Updated hw.modbus.parity");
    }
    else if (key == "modbus.interval_ms") {
        cfg.hardware.modbus_rs485.interval_ms = value.toInt();
        printSuccess("Updated hw.modbus.interval_ms");
    }
    else if (key == "modbus.max_resp_ms") {
        cfg.hardware.modbus_rs485.max_resp_ms = value.toInt();
        printSuccess("Updated hw.modbus.max_resp_ms");
    }
    else if (key == "modbus.max_retry") {
        cfg.hardware.modbus_rs485.max_retry = value.toInt();
        printSuccess("Updated hw.modbus.max_retry");
    }
    // ---------- Modbus Channels ----------
    else if (key.startsWith("modbus.channel[")) {
        int braceOpen = key.indexOf('[');
        int braceClose = key.indexOf(']');
        if (braceOpen == -1 || braceClose == -1 || braceClose <= braceOpen) {
            printError("Invalid channel format. Use modbus.channel[<idx>].<field>");
            return;
        }
        String idxStr = key.substring(braceOpen + 1, braceClose);
        int idx = idxStr.toInt();
        if (idx < 0 || idx >= MAX_MODBUS_CHANNELS) {
            printError("Channel index out of range");
            return;
        }
        String field = key.substring(braceClose + 2); // skip "].<field>"

        if (field == "id") {
            cfg.hardware.modbus_rs485.channels[idx].id = value.toInt();
            String msg = "Updated modbus.channel[" + idxStr + "].id";
            printSuccess(msg.c_str());
        }
        else if (field == "name") {
            strncpy(cfg.hardware.modbus_rs485.channels[idx].name, value.c_str(),
                    sizeof(cfg.hardware.modbus_rs485.channels[idx].name) - 1);
            cfg.hardware.modbus_rs485.channels[idx].name[sizeof(cfg.hardware.modbus_rs485.channels[idx].name) - 1] = '\0';
            String msg = "Updated modbus.channel[" + idxStr + "].name";
            printSuccess(msg.c_str());
        }
        else if (field == "slave_id") {
            cfg.hardware.modbus_rs485.channels[idx].slave_id = value.toInt();
            String msg = "Updated modbus.channel[" + idxStr + "].slave_id";
            printSuccess(msg.c_str());
        }
        else if (field == "address") {
            cfg.hardware.modbus_rs485.channels[idx].address = value.toInt();
            String msg = "Updated modbus.channel[" + idxStr + "].address";
            printSuccess(msg.c_str());
        }
        else if (field == "quantity") {
            cfg.hardware.modbus_rs485.channels[idx].quantity = value.toInt();
            String msg = "Updated modbus.channel[" + idxStr + "].quantity";
            printSuccess(msg.c_str());
        }
        else if (field == "type") {
            strncpy(cfg.hardware.modbus_rs485.channels[idx].type, value.c_str(),
                    sizeof(cfg.hardware.modbus_rs485.channels[idx].type) - 1);
            cfg.hardware.modbus_rs485.channels[idx].type[sizeof(cfg.hardware.modbus_rs485.channels[idx].type) - 1] = '\0';
            String msg = "Updated modbus.channel[" + idxStr + "].type";
            printSuccess(msg.c_str());
        }
        else if (field == "byte_order") {
            strncpy(cfg.hardware.modbus_rs485.channels[idx].byte_order, value.c_str(),
                    sizeof(cfg.hardware.modbus_rs485.channels[idx].byte_order) - 1);
            cfg.hardware.modbus_rs485.channels[idx].byte_order[sizeof(cfg.hardware.modbus_rs485.channels[idx].byte_order) - 1] = '\0';
            String msg = "Updated modbus.channel[" + idxStr + "].byte_order";
            printSuccess(msg.c_str());
        }
        else {
            String errMsg = "Unknown channel field: " + field;
            printError(errMsg.c_str());
        }
    }
    else {
        printError("Unknown key in 'hw'. Available: i2c.*, modbus.*, modbus.channel[<idx>].*");
    }
}

void SerialCLI::handleSetLogging(String key, String value) {
    DeviceConfig& cfg = _cfgMgr->get();
    
    if (key == "enabled") {
        cfg.logging.enabled = (value == "1" || value == "true");
        printSuccess("Updated log.enabled");
    } else if (key == "level") {
        strncpy(cfg.logging.level, value.c_str(), sizeof(cfg.logging.level) - 1);
        printSuccess("Updated log.level");
    } else if (key == "sd_log") {
        cfg.logging.sd_log = (value == "1" || value == "true");
        printSuccess("Updated log.sd_log");
    } else {
        printError("Unknown key in 'log'. Try: enabled, level, sd_log");
    }
}

void SerialCLI::handleSetComm(String key, String value) {
    DeviceConfig& cfg = _cfgMgr->get();
    
    if (key == "serial.baud") {
        cfg.communication.serial.baud = value.toInt();
        printSuccess("Updated comm.serial.baud");
    } else if (key == "rs485.baud") {
        cfg.communication.rs485.baud = value.toInt();
        printSuccess("Updated comm.rs485.baud");
    } else {
        printError("Unknown key in 'comm'. Try: serial.baud, rs485.baud");
    }
}

void SerialCLI::handleSetLoRa(String key, String value) {
    DeviceConfig& cfg = _cfgMgr->get();

    // === General LoRa ===
    if (key == "enabled") {
        cfg.lora.enabled = (value == "1" || value == "true");
        printSuccess("Updated lora.enabled");
    }
    // === LoRaWAN Common ===
    else if (key == "lorawan.mode") {
        strncpy(cfg.lora.lorawan.mode, value.c_str(), sizeof(cfg.lora.lorawan.mode) - 1);
        cfg.lora.lorawan.mode[sizeof(cfg.lora.lorawan.mode) - 1] = '\0';
        printSuccess("Updated lora.lorawan.mode");
    }
    else if (key == "lorawan.class_type") {
        strncpy(cfg.lora.lorawan.class_type, value.c_str(), sizeof(cfg.lora.lorawan.class_type) - 1);
        cfg.lora.lorawan.class_type[sizeof(cfg.lora.lorawan.class_type) - 1] = '\0';
        printSuccess("Updated lora.lorawan.class_type");
    }
    else if (key == "lorawan.class_c_continuous_rx") {
        cfg.lora.lorawan.class_c_continuous_rx = (value == "1" || value == "true");
        printSuccess("Updated lora.lorawan.class_c_continuous_rx");
    }
    else if (key == "lorawan.uplink_interval_sec") {
        cfg.lora.lorawan.uplink_interval_sec = value.toInt();
        printSuccess("Updated lora.lorawan.uplink_interval_sec");
    }
    else if (key == "lorawan.tx_sf") {
        cfg.lora.lorawan.tx_sf = value.toInt();
        printSuccess("Updated lora.lorawan.tx_sf");
    }
    else if (key == "lorawan.tx_power") {
        cfg.lora.lorawan.tx_power = value.toInt();
        printSuccess("Updated lora.lorawan.tx_power");
    }
    else if (key == "lorawan.tx_adr") {
        cfg.lora.lorawan.tx_adr = (value == "1" || value == "true");
        printSuccess("Updated lora.lorawan.tx_adr");
    }
    else if (key == "lorawan.duty_cycle") {
        cfg.lora.lorawan.duty_cycle = (value == "1" || value == "true");
        printSuccess("Updated lora.lorawan.duty_cycle");
    }
    else if (key == "lorawan.fport") {
        cfg.lora.lorawan.fport = value.toInt();
        printSuccess("Updated lora.lorawan.fport");
    }
    else if (key == "lorawan.rx1_delay_ms") {
        cfg.lora.lorawan.rx1_delay_ms = value.toInt();
        printSuccess("Updated lora.lorawan.rx1_delay_ms");
    }
    else if (key == "lorawan.rx1_data_rate") {
        strncpy(cfg.lora.lorawan.rx1_data_rate, value.c_str(), sizeof(cfg.lora.lorawan.rx1_data_rate) - 1);
        cfg.lora.lorawan.rx1_data_rate[sizeof(cfg.lora.lorawan.rx1_data_rate) - 1] = '\0';
        printSuccess("Updated lora.lorawan.rx1_data_rate");
    }
    else if (key == "lorawan.rx2_frequency") {
        cfg.lora.lorawan.rx2_frequency = value.toInt();
        printSuccess("Updated lora.lorawan.rx2_frequency");
    }
    else if (key == "lorawan.rx2_data_rate") {
        strncpy(cfg.lora.lorawan.rx2_data_rate, value.c_str(), sizeof(cfg.lora.lorawan.rx2_data_rate) - 1);
        cfg.lora.lorawan.rx2_data_rate[sizeof(cfg.lora.lorawan.rx2_data_rate) - 1] = '\0';
        printSuccess("Updated lora.lorawan.rx2_data_rate");
    }
    else if (key == "lorawan.confirmed_uplink") {
        cfg.lora.lorawan.confirmed_uplink = (value == "1" || value == "true");
        printSuccess("Updated lora.lorawan.confirmed_uplink");
    }

    // === OTAA Keys ===
    else if (key == "otaa.join_eui") {
        strncpy(cfg.lora.lorawan.otaa.join_eui, value.c_str(), sizeof(cfg.lora.lorawan.otaa.join_eui) - 1);
        cfg.lora.lorawan.otaa.join_eui[sizeof(cfg.lora.lorawan.otaa.join_eui) - 1] = '\0';
        printSuccess("Updated OTAA join_eui");
    }
    else if (key == "otaa.dev_eui") {
        strncpy(cfg.lora.lorawan.otaa.dev_eui, value.c_str(), sizeof(cfg.lora.lorawan.otaa.dev_eui) - 1);
        cfg.lora.lorawan.otaa.dev_eui[sizeof(cfg.lora.lorawan.otaa.dev_eui) - 1] = '\0';
        printSuccess("Updated OTAA dev_eui");
    }
    else if (key == "otaa.app_key") {
        strncpy(cfg.lora.lorawan.otaa.app_key, value.c_str(), sizeof(cfg.lora.lorawan.otaa.app_key) - 1);
        cfg.lora.lorawan.otaa.app_key[sizeof(cfg.lora.lorawan.otaa.app_key) - 1] = '\0';
        printSuccess("Updated OTAA app_key");
    }
    else if (key == "otaa.nwk_key") {
        strncpy(cfg.lora.lorawan.otaa.nwk_key, value.c_str(), sizeof(cfg.lora.lorawan.otaa.nwk_key) - 1);
        cfg.lora.lorawan.otaa.nwk_key[sizeof(cfg.lora.lorawan.otaa.nwk_key) - 1] = '\0';
        printSuccess("Updated OTAA nwk_key");
    }

    // === ABP Keys ===
    else if (key == "abp.dev_addr") {
        strncpy(cfg.lora.lorawan.abp.dev_addr, value.c_str(), sizeof(cfg.lora.lorawan.abp.dev_addr) - 1);
        cfg.lora.lorawan.abp.dev_addr[sizeof(cfg.lora.lorawan.abp.dev_addr) - 1] = '\0';
        printSuccess("Updated ABP dev_addr");
    }
    else if (key == "abp.nwk_skey") {
        strncpy(cfg.lora.lorawan.abp.nwk_skey, value.c_str(), sizeof(cfg.lora.lorawan.abp.nwk_skey) - 1);
        cfg.lora.lorawan.abp.nwk_skey[sizeof(cfg.lora.lorawan.abp.nwk_skey) - 1] = '\0';
        printSuccess("Updated ABP nwk_skey");
    }
    else if (key == "abp.app_skey") {
        strncpy(cfg.lora.lorawan.abp.app_skey, value.c_str(), sizeof(cfg.lora.lorawan.abp.app_skey) - 1);
        cfg.lora.lorawan.abp.app_skey[sizeof(cfg.lora.lorawan.abp.app_skey) - 1] = '\0';
        printSuccess("Updated ABP app_skey");
    }
    else {
        printError("Unknown key in 'lora'. Check 'show lora' for available keys.");
    }
}

// ======================== Show Configuration ========================
/**
 * @brief Display current configuration based on category.
 * @param category "all", "device", "lora", "hw", "comm", "log", or "sd".
 */
void SerialCLI::showConfig(String category) {
    category.toLowerCase();
    
    _serial->println(CLI_COLOR_BOLD "\n--- Configuration ---" CLI_COLOR_RESET);
    if (category == "sd" || category == "sdconfig" || category == "sd.json") {
        showSDConfig();
        _serial->println(CLI_COLOR_BOLD "---------------------\n" CLI_COLOR_RESET);
        return;
    }
    if (category == "" || category == "all" || category == "device") {
        showDeviceConfig();
    }
    if (category == "" || category == "all" || category == "lora") {
        showLoRaConfig();
    }
    if (category == "" || category == "all" || category == "hw") {
        showHardwareConfig();
    }
    if (category == "" || category == "all" || category == "comm" || category == "communication") {
        showCommunicationConfig();
    }
    if (category == "" || category == "all" || category == "log" || category == "logging") {
        showLoggingConfig();
    }
    _serial->println(CLI_COLOR_BOLD "---------------------\n" CLI_COLOR_RESET);
}

void SerialCLI::showSDConfig() {
    if (!_sd) { printError("SD not available"); return; }

    const char* cfgName = _sd->getConfig();
    _serial->printf("SD Config file: %s\n", cfgName);
    String content = _sd->readFile(cfgName);
    if (content == "ERROR_OPEN") { printError("Open failed"); return; }

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, content);
    if (err) {
        _serial->println("JSON parse error");
        _serial->println(content);
        return;
    }

    serializeJsonPretty(doc, *_serial);
    _serial->println();
}


void SerialCLI::showDeviceConfig() {
    DeviceConfig& cfg = _cfgMgr->get();
    _serial->println(CLI_COLOR_CYAN "[Device Information]" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "device.name", cfg.device.name[0] ? cfg.device.name : CLI_COLOR_RED "(empty)" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "device.type", cfg.device.type[0] ? cfg.device.type : CLI_COLOR_RED "(empty)" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "device.id", cfg.device.id[0] ? cfg.device.id : CLI_COLOR_RED "(empty)" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "device.uid", cfg.device.uid[0] ? cfg.device.uid : CLI_COLOR_RED "(empty)" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "device.use_sd_config", cfg.device.use_sd_config ? CLI_COLOR_GREEN "SD Card" : CLI_COLOR_YELLOW "EEPROM");
}

void SerialCLI::showLoRaConfig() {
    DeviceConfig& cfg = _cfgMgr->get();
    _serial->println(CLI_COLOR_CYAN "\n[LoRaWAN Settings]" CLI_COLOR_RESET);
    
    // main settings
    _serial->printf("  %-22s : %s\n", "lora.enabled", cfg.lora.enabled ? CLI_COLOR_GREEN "Yes" CLI_COLOR_RESET : CLI_COLOR_RED "No" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "lora.region", cfg.lora.region[0] ? cfg.lora.region : CLI_COLOR_RED "(empty)" CLI_COLOR_RESET);
    // LoRa pins
    _serial->printf("  %-22s : SS=%s, RST=%s, DIO0=%s, DIO1=%s\n", "lora.pins", cfg.lora.pins_ss, cfg.lora.pins_rst, cfg.lora.pins_dio0, cfg.lora.pins_dio1);
    
    _serial->printf("  %-22s : %s\n", "lora.lorawan.mode", cfg.lora.lorawan.mode[0] ? cfg.lora.lorawan.mode : CLI_COLOR_RED "(empty)" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "lora.lorawan.class_type", cfg.lora.lorawan.class_type);
    if (cfg.lora.lorawan.class_type[0] == 'C') {
        _serial->printf("  %-22s : %s\n", "lora.class_c_continuous_rx", cfg.lora.lorawan.class_c_continuous_rx ? "Yes" : "No");
    } else if (cfg.lora.lorawan.class_type[0] == 'A') {
        _serial->printf("  %-22s : %lu ms\n", "lora.class_a.rx1_delay_ms", cfg.lora.lorawan.rx1_delay_ms);
        _serial->printf("  %-22s : %s\n", "lora.class_a.rx1_data_rate", cfg.lora.lorawan.rx1_data_rate);
        #ifdef HAS_UPLINK_INTERVAL_MIN
        _serial->printf("  %-22s : %lu sec\n", "lora.class_a.uplink_interval_min", cfg.lora.lorawan.uplink_interval_min);
        #endif
    }
    _serial->printf("  %-22s : %lu sec\n", "lora.lorawan.uplink_interval_sec", cfg.lora.lorawan.uplink_interval_sec);
    
    // RX2
    _serial->printf("  %-22s : %lu Hz\n", "lora.rx2.frequency", cfg.lora.lorawan.rx2_frequency);
    _serial->printf("  %-22s : %s\n", "lora.rx2.data_rate", cfg.lora.lorawan.rx2_data_rate);
    
    // TX parameters
    _serial->printf("  %-22s : SF=%d, Power=%d dBm, ADR=%s\n", "lora.tx", 
        cfg.lora.lorawan.tx_sf, cfg.lora.lorawan.tx_power,
        cfg.lora.lorawan.tx_adr ? "ON" : "OFF");
    _serial->printf("  %-22s : %s\n", "lora.lorawan.confirmed_uplink", cfg.lora.lorawan.confirmed_uplink ? "Yes" : "No");
    _serial->printf("  %-22s : %d\n", "lora.lorawan.fport", cfg.lora.lorawan.fport);
    _serial->printf("  %-22s : %s\n", "lora.lorawan.duty_cycle", cfg.lora.lorawan.duty_cycle ? "Enabled" : "Disabled");

    // OTAA Keys
    _serial->println(CLI_COLOR_YELLOW "  --- OTAA Keys ---" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "lora.otaa.join_eui", cfg.lora.lorawan.otaa.join_eui[0] ? cfg.lora.lorawan.otaa.join_eui : "(not set)");
    _serial->printf("  %-22s : %s\n", "lora.otaa.dev_eui", cfg.lora.lorawan.otaa.dev_eui[0] ? cfg.lora.lorawan.otaa.dev_eui : "(not set)");
    _serial->printf("  %-22s : %s\n", "lora.otaa.nwk_key", cfg.lora.lorawan.otaa.nwk_key[0] ? cfg.lora.lorawan.otaa.nwk_key : "(not set)");
    _serial->printf("  %-22s : %s\n", "lora.otaa.app_key", cfg.lora.lorawan.otaa.app_key[0] ? cfg.lora.lorawan.otaa.app_key : "(not set)");
    
    // ABP Keys
    _serial->println(CLI_COLOR_YELLOW "  --- ABP Keys ---" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "lora.abp.dev_addr", cfg.lora.lorawan.abp.dev_addr[0] ? cfg.lora.lorawan.abp.dev_addr : "(not set)");
    _serial->printf("  %-22s : %s\n", "lora.abp.nwk_skey", cfg.lora.lorawan.abp.nwk_skey[0] ? cfg.lora.lorawan.abp.nwk_skey : "(not set)");
    _serial->printf("  %-22s : %s\n", "lora.abp.app_skey", cfg.lora.lorawan.abp.app_skey[0] ? cfg.lora.lorawan.abp.app_skey : "(not set)");
}
void SerialCLI::showHardwareConfig() {
    DeviceConfig& cfg = _cfgMgr->get();
    
    // ========== I2C ==========
    _serial->println(CLI_COLOR_CYAN "\n[Hardware - I2C]" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "hw.i2c.enable", cfg.hardware.i2c.enable ? "Yes" : "No");
    _serial->printf("  %-22s : %lu Hz\n", "hw.i2c.freq", cfg.hardware.i2c.frequency);
    _serial->printf("  %-22s : 0x%02X\n", "hw.i2c.master_address", cfg.hardware.i2c.master_address);
    _serial->printf("  %-22s : %lu ms\n", "hw.i2c.interval", cfg.hardware.i2c.interval_ms);
    _serial->printf("  %-22s : %lu ms\n", "hw.i2c.max_resp_ms", cfg.hardware.i2c.max_resp_ms);
    _serial->printf("  %-22s : %d\n", "hw.i2c.max_retry", cfg.hardware.i2c.max_retry);

    const size_t maxDev = sizeof(cfg.hardware.i2c.devices) / sizeof(cfg.hardware.i2c.devices[0]);
    for (size_t d = 0; d < maxDev; d++) {
        if (cfg.hardware.i2c.devices[d].id == 0) continue;
        _serial->printf("  I2C Device #%d: id=%d, name=%s, addr=0x%02X\n", d+1,
            cfg.hardware.i2c.devices[d].id,
            cfg.hardware.i2c.devices[d].name,
            cfg.hardware.i2c.devices[d].address);
        const size_t maxCh = sizeof(cfg.hardware.i2c.devices[d].channels) / sizeof(cfg.hardware.i2c.devices[d].channels[0]);
        for (size_t ch = 0; ch < maxCh; ch++) {
            if (cfg.hardware.i2c.devices[d].channels[ch].id == 0) continue;
            char scaleBuf[10];
            dtostrf(cfg.hardware.i2c.devices[d].channels[ch].scale, 5, 2, scaleBuf);
            _serial->printf("    Channel #%d: %s, reg=0x%02X, len=%d, order=%s, scale=%s\n",
                cfg.hardware.i2c.devices[d].channels[ch].id,
                cfg.hardware.i2c.devices[d].channels[ch].name,
                cfg.hardware.i2c.devices[d].channels[ch].reg_addr,
                cfg.hardware.i2c.devices[d].channels[ch].length,
                cfg.hardware.i2c.devices[d].channels[ch].byte_order,
                scaleBuf);
        }
    }

    // OLED
    _serial->println(CLI_COLOR_CYAN "\n[Hardware - OLED]" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "hw.oled.enabled", cfg.hardware.oled.enabled ? CLI_COLOR_GREEN "Yes" CLI_COLOR_RESET : CLI_COLOR_RED "No" CLI_COLOR_RESET);
    _serial->printf("  %-22s : 0x%02X\n", "hw.oled.address", cfg.hardware.oled.address);
    
    // LED
    _serial->println(CLI_COLOR_CYAN "\n[Hardware - LED]" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "hw.led.active_low", cfg.hardware.led.active_low ? "Yes" : "No");
    
    // Analog Input
    _serial->println(CLI_COLOR_CYAN "\n[Hardware - Analog Input]" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "hw.analog.enable", cfg.hardware.analog.enable ? CLI_COLOR_GREEN "Yes" CLI_COLOR_RESET : CLI_COLOR_RED "No" CLI_COLOR_RESET);
    if (cfg.hardware.analog.enable) {
        char minBuf[8], maxBuf[8], slopeBuf[8], interBuf[8];
        dtostrf(cfg.hardware.analog.scale_min, 4, 1, minBuf);
        dtostrf(cfg.hardware.analog.scale_max, 4, 1, maxBuf);
        dtostrf(cfg.hardware.analog.factor_slope, 5, 2, slopeBuf);
        dtostrf(cfg.hardware.analog.factor_intercept, 5, 2, interBuf);
        _serial->printf("  %-22s : %s - %s mA\n", "hw.analog.scale", minBuf, maxBuf);
        _serial->printf("  %-22s : y = %s * x + %s\n", "hw.analog.factor", slopeBuf, interBuf);
    }
    
    // LS_SW
    _serial->println(CLI_COLOR_CYAN "\n[Hardware - LS_SW]" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "hw.ls_sw.enable", cfg.hardware.ls_sw.enable ? CLI_COLOR_GREEN "Yes" CLI_COLOR_RESET : CLI_COLOR_RED "No" CLI_COLOR_RESET);
    if (cfg.hardware.ls_sw.enable) {
        _serial->printf("  %-22s : %s\n", "hw.ls_sw.inverted", cfg.hardware.ls_sw.inverted ? "Yes" : "No");
        _serial->printf("  %-22s : %s\n", "hw.ls_sw.default_state", cfg.hardware.ls_sw.default_state);
        _serial->printf("  %-22s : %s\n", "hw.ls_sw.mode", cfg.hardware.ls_sw.mode);
        _serial->printf("  %-22s : %s\n", "hw.ls_sw.pull", cfg.hardware.ls_sw.pull);
        _serial->printf("  %-22s : %s\n", "hw.ls_sw.speed", cfg.hardware.ls_sw.speed);
        _serial->printf("  %-22s : %lu ms\n", "hw.ls_sw.startup_delay_ms", cfg.hardware.ls_sw.startup_delay_ms);
    }
    
    // SHT3
    _serial->println(CLI_COLOR_CYAN "\n[Hardware - SHT3]" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "hw.sht3.enable", cfg.hardware.sht3.enable ? CLI_COLOR_GREEN "Yes" CLI_COLOR_RESET : CLI_COLOR_RED "No" CLI_COLOR_RESET);
    if (cfg.hardware.sht3.enable) {
        _serial->printf("  %-22s : 0x%02X\n", "hw.sht3.address", cfg.hardware.sht3.address);
    }
    
    // Modbus RS485
    _serial->println(CLI_COLOR_CYAN "\n[Hardware - Modbus RS485]" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "hw.modbus.enable", cfg.hardware.modbus_rs485.enable ? CLI_COLOR_GREEN "Yes" CLI_COLOR_RESET : CLI_COLOR_RED "No" CLI_COLOR_RESET);
    if (cfg.hardware.modbus_rs485.enable) {
        _serial->printf("  %-22s : %lu\n", "hw.modbus.baud_rate", cfg.hardware.modbus_rs485.baud_rate);
        _serial->printf("  %-22s : %d\n", "hw.modbus.data_bit", cfg.hardware.modbus_rs485.data_bit);
        _serial->printf("  %-22s : %d\n", "hw.modbus.stop_bit", cfg.hardware.modbus_rs485.stop_bit);
        _serial->printf("  %-22s : %s\n", "hw.modbus.parity", cfg.hardware.modbus_rs485.parity);
        _serial->printf("  %-22s : %lu ms\n", "hw.modbus.interval_ms", cfg.hardware.modbus_rs485.interval_ms);
        _serial->printf("  %-22s : %lu ms\n", "hw.modbus.max_resp_ms", cfg.hardware.modbus_rs485.max_resp_ms);
        _serial->printf("  %-22s : %d\n", "hw.modbus.max_retry", cfg.hardware.modbus_rs485.max_retry);
        
        for (int ch = 0; ch < MAX_MODBUS_CHANNELS; ch++) {
            if (cfg.hardware.modbus_rs485.channels[ch].id == 0) continue;
            _serial->printf("    Channel #%d: %s, slave=%d, addr=%d, qty=%d, type=%s, order=%s, sign=%s\n",
                cfg.hardware.modbus_rs485.channels[ch].id,
                cfg.hardware.modbus_rs485.channels[ch].name,
                cfg.hardware.modbus_rs485.channels[ch].slave_id,
                cfg.hardware.modbus_rs485.channels[ch].address,
                cfg.hardware.modbus_rs485.channels[ch].quantity,
                cfg.hardware.modbus_rs485.channels[ch].type,
                cfg.hardware.modbus_rs485.channels[ch].byte_order,
                cfg.hardware.modbus_rs485.channels[ch].sign ? "true" : "false");
        }
    }
}
void SerialCLI::showCommunicationConfig() {
    DeviceConfig& cfg = _cfgMgr->get();
    _serial->println(CLI_COLOR_CYAN "\n[Communication Settings]" CLI_COLOR_RESET);
    
    // Serial
    _serial->println(CLI_COLOR_YELLOW "  --- Serial ---" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "comm.serial.port", cfg.communication.serial.port);
    _serial->printf("  %-22s : %lu baud\n", "comm.serial.baud", cfg.communication.serial.baud);
    _serial->printf("  %-22s : %lu ms\n", "comm.serial.timeout", cfg.communication.serial.timeout);
    
    // RS485
    _serial->println(CLI_COLOR_YELLOW "  --- RS485 ---" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "comm.rs485.port", cfg.communication.rs485.port);
    _serial->printf("  %-22s : %lu baud\n", "comm.rs485.baud", cfg.communication.rs485.baud);
    _serial->printf("  %-22s : %lu ms\n", "comm.rs485.timeout", cfg.communication.rs485.timeout);
    _serial->printf("  %-22s : %d bits\n", "comm.rs485.data_bits", cfg.communication.rs485.data_bits);
    _serial->printf("  %-22s : %d bits\n", "comm.rs485.stop_bits", cfg.communication.rs485.stop_bits);
    _serial->printf("  %-22s : %s\n", "comm.rs485.parity", cfg.communication.rs485.parity);
    
    // // RS485 slaves 
    // #ifdef HAS_RS485_SLAVES
    // _serial->printf("  %-22s : ", "comm.rs485.slaves");
    // for (uint8_t i = 0; i < cfg.communication.rs485.slave_count; i++) {
    //     if (i > 0) _serial->print(", ");
    //     _serial->print(cfg.communication.rs485.slaves[i]);
    // }
    // if (cfg.communication.rs485.slave_count == 0) _serial->print("(none)");
    // _serial->println();
    // #endif
}

void SerialCLI::showLoggingConfig() {
    DeviceConfig& cfg = _cfgMgr->get();
    _serial->println(CLI_COLOR_CYAN "\n[Logging Settings]" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "logging.enabled", cfg.logging.enabled ? CLI_COLOR_GREEN "Yes" CLI_COLOR_RESET : CLI_COLOR_RED "No" CLI_COLOR_RESET);
    _serial->printf("  %-22s : %s\n", "logging.level", cfg.logging.level);
    _serial->printf("  %-22s : %s\n", "logging.sd_log", cfg.logging.sd_log ? CLI_COLOR_GREEN "Yes" CLI_COLOR_RESET : CLI_COLOR_RED "No" CLI_COLOR_RESET);
}
// ================= SYSTEM COMMANDS =================
void SerialCLI::printHelp() {
    _serial->println(CLI_COLOR_BOLD "\nAvailable Commands:" CLI_COLOR_RESET);
    _serial->println("  " CLI_COLOR_YELLOW "show [all|device|lora|hw]" CLI_COLOR_RESET " : Show configuration");
    _serial->println("  " CLI_COLOR_YELLOW "set <category.key> <value>" CLI_COLOR_RESET "  : Set a value");
    _serial->println("      Examples: set device.name MyNode, set lora.mode OTAA");
    _serial->println("  " CLI_COLOR_YELLOW "save" CLI_COLOR_RESET "                          : Save to EEPROM");
    _serial->println("  " CLI_COLOR_YELLOW "factory-reset" CLI_COLOR_RESET "                 : Reset all config");
    _serial->println("  " CLI_COLOR_YELLOW "reboot" CLI_COLOR_RESET "                        : Restart system\n");
    _serial->println("  " CLI_COLOR_YELLOW "sd.config" CLI_COLOR_RESET "                     : Show config from SD card");
     _serial->println("  " CLI_COLOR_YELLOW "sd.list" CLI_COLOR_RESET "                       : List files on SD card");
     _serial->println("  " CLI_COLOR_YELLOW "sd.read <path>" CLI_COLOR_RESET "                 : Read file from SD card");
     _serial->println("  " CLI_COLOR_YELLOW "toggle" CLI_COLOR_RESET "                    : Toggle LS Switch (if available)");
     _serial->println("  " CLI_COLOR_YELLOW "i2c scan" CLI_COLOR_RESET "                    : Scan I2C bus for devices");
     _serial->println();
}

void SerialCLI::rebootSystem() {
    _serial->println(CLI_COLOR_YELLOW "Rebooting..." CLI_COLOR_RESET);
    delay(100);
    NVIC_SystemReset();
}

void SerialCLI::factoryReset() {
    _serial->print(CLI_COLOR_RED "Factory Resetting... " CLI_COLOR_RESET);
    _cfgMgr->factoryReset();
    _serial->println("Done.");
    rebootSystem();
}

// ================= UI HELPERS =================

void SerialCLI::printPrompt() {
    _serial->print(CLI_COLOR_CYAN "stm32-node> " CLI_COLOR_RESET);
}

void SerialCLI::printSuccess(const char* msg) {
    _serial->print(CLI_COLOR_GREEN "[OK] " CLI_COLOR_RESET);
    _serial->println(msg);
}

void SerialCLI::printError(const char* msg) {
    _serial->print(CLI_COLOR_RED "[ERROR] " CLI_COLOR_RESET);
    _serial->println(msg);
}