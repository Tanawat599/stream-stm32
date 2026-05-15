# LoRa Node Firmware for STM32F103

[![PlatformIO](https://img.shields.io/badge/PlatformIO-STM32-blue)](https://platformio.org/)
[![LoRaWAN](https://img.shields.io/badge/LoRaWAN-1.0.3-green)](https://lora-alliance.org/)

Firmware for an industrial IoT LoRaWAN node with multi-sensor support (I2C, Modbus RTU, SHT3, 4-20mA), SD card logging, OLED display, and a full serial CLI.

Built for **STM32F103C8/CB (Blue Pill)** using PlatformIO.

---

# Features

- **LoRaWAN 1.0.3**
  - ABP / OTAA
  - Class A/C
  - Adaptive Data Rate (ADR)
  - Confirmed / unconfirmed uplinks

- **Multi-sensor acquisition**
  - I2C master – up to 8 devices, each with 8 channels
  - Modbus RTU master (RS485)
  - On-board SHT3 temperature & humidity sensor
  - 4-20mA analog input with voltage scaling

- **Low-side switch output**
  - Configurable logic
  - Open-drain / push-pull
  - Startup delay support

- **OLED display**
  - 128×64 SSD1306 over I2C
  - System status & downlink message display

- **SD card support**
  - FAT32
  - Store `config.json`
  - Event logging (uplink/downlink/errors)

- **Serial CLI**
  - VT100 compatible
  - 115200 baud

- **Battery monitoring**
  - Voltage
  - Current sensing
  - TP4056 charging status

- **Watchdog**
  - Automatic reboot using IWDG

---

# Source File Descriptions

| File | Description |
|---|---|
| `cli/serial_cli.cpp` | Implements the serial command line interface (CLI) for configuration, debugging, and runtime control over UART. |
| `communication/i2c.cpp` | Handles I2C communication, sensor polling, device scanning, and register-based data acquisition. |
| `communication/modbus_rs485.cpp` | Implements Modbus RTU master communication over RS485 for external industrial sensors and slave devices. |
| `config/config.cpp` | Loads, parses, validates, and stores system configuration from EEPROM or SD card (`config.json`). |
| `display/oled.cpp` | Controls the SSD1306 OLED display and renders system status, sensor values, and downlink messages. |
| `io/analog.cpp` | Reads analog inputs (0–3.3V / 4–20mA) using the STM32 ADC and applies scaling/conversion logic. |
| `io/ls_switch.cpp` | Controls the low-side switch output, including startup behavior and output state management. |
| `io/sht3_sensor.cpp` | Reads temperature and humidity data from the onboard SHT3 sensor via I2C. |
| `lora/lorap2p.cpp` | Provides LoRa point-to-point (P2P) communication support for non-LoRaWAN operation. |
| `lora/lorawan.cpp` | Implements LoRaWAN functionality including OTAA/ABP join, uplink/downlink handling, ADR, and MAC processing. |
| `sd_card/sdcard.cpp` | Handles SD card initialization, file operations, configuration loading, and event logging. |
| `main.cpp` | Main firmware entry point. Initializes peripherals, loads configuration, and runs the primary application loop. |

---

# Downlink Commands

| Command | Description | Example |
|---|---|---|
| `0x01` | Control LED | `0101` = ON, `0100` = OFF |
| `0x02` | Control Low-Side Switch | `020101` |
| `0x03` | Display text on OLED | `0348656C6C6F` → "Hello" |

---

# Hardware Requirements & Pin Mapping

| Component | STM32F103 Pin | Notes |
|---|---|---|
| LoRa NSS | `PB11` | |
| LoRa RST | `PB12` | |
| LoRa DIO0 | `PB0` | |
| SD CS | `PA8` | Verify conflict with LoRa reset |
| SD SCK | `PB13` | |
| SD MISO | `PB14` | |
| SD MOSI | `PB15` | |
| I2C SCL | `PB6` | 4.7kΩ pull-up required |
| I2C SDA | `PB7` | 4.7kΩ pull-up required |
| RS485 DE | `PB9` | |
| RS485 RE | `PB8` | |
| Analog Input | `PA4` | 0–3.3V ADC |
| Low-Side Switch | `PB5` | |
| LED (active low) | `PA12` | On-board LED |
| Button (optional) | `PC13` | Enter CLI mode |

> Verify all pin definitions in `config.h` before deployment.

---

# Project Structure

```text
stm32f103
.
├── include
│   ├── README
│   ├── config.h
│   ├── device_config.h
│   └── mylib.h
├── lib
│   └── README
├── platformio.ini
├── sd_card
│   └── config.json
├── src
│   ├── backup
│   │   └── *.bak
│   ├── cli
│   │   └── serial_cli.cpp
│   ├── communication
│   │   ├── i2c.cpp
│   │   └── modbus_rs485.cpp
│   ├── config
│   │   └── config.cpp
│   ├── display
│   │   └── oled.cpp
│   ├── io
│   │   ├── analog.cpp
│   │   ├── ls_switch.cpp
│   │   └── sht3_sensor.cpp
│   ├── lora
│   │   ├── lorap2p.cpp
│   │   └── lorawan.cpp
│   ├── sd_card
│   │   └── sdcard.cpp
│   ├── main.cpp
│   └── stm32f103.code-workspace
└── test
    └── README
```

---

# Configuration Sources

The firmware supports two configuration sources:

1. **EEPROM (default)**
   - Saved via CLI commands
   - Example: `set ...` → `save`

2. **SD Card**
   - Enable using:
     ```cpp
     device.use_sd_config = true;
     ```
   - Reads `/config.json` from SD card root
   - Overrides EEPROM settings

---

# Documentation

Additional project documentation, architecture notes, and configuration references are available here:

- [Project Documentation](https://1drv.ms/w/c/8a9037343536ec56/IQDEORJlU61uTK86r0NgJ6iZAT8GpFkhxgMwAoE4rJOQ1-M?e=SzrAQA)


---

# Example `config.json`

```json
{
  "hardware": {
    "i2c": {
      "enable": true,
      "frequency": 100000,
      "interval_ms": 2000,
      "devices": [
        {
          "id": 1,
          "name": "sensor_1",
          "address": "0x40",
          "channels": [
            {
              "id": 1,
              "name": "temperature",
              "register": "0x00",
              "length": 2,
              "byte_order": "AB",
              "scale": 0.1
            }
          ]
        }
      ]
    }
  }
}
```