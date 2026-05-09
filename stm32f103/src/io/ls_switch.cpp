#include "mylib.h"
#include "config.h"

static bool parsePin(const String& pinStr, GPIO_TypeDef*& port, uint16_t& pin)
{
    if (pinStr.length() < 3) return false;

    char portChar = pinStr[1];
    int pinNum = pinStr.substring(2).toInt();

    if (pinNum < 0 || pinNum > 15) return false;  

    switch (portChar) {
        case 'A': port = GPIOA; break;
        case 'B': port = GPIOB; break;
        case 'C': port = GPIOC; break;
        default: return false;
    }

    pin = (uint16_t)(1U << pinNum);  

    return true;
}

LowSideSwitch::LowSideSwitch()
{
    memset(&conf, 0, sizeof(conf));
}

bool LowSideSwitch::loadConfigFromJson(const JsonObject& sw)
{


    conf.ENABLE = sw["enable"] | false;
    conf.INVERTED = sw["inverted"] | false;
    conf.STARTUP_DELAY = sw["startup_delay_ms"] | 0;

    // default state
    String def = sw["default_state"] | "off";
    conf.DEFAULT_STATE = (def == "on") ? GPIO_PIN_SET : GPIO_PIN_RESET;

    // pin
    String pinStr = sw["pin"] | "PB5";
    if (!parsePin(pinStr, conf.PORT, conf.PIN)) {
        Serial1.println(F("LS_SW: invalid pin"));
        return false;
    }

    // mode
    String mode = sw["mode"] | "push_pull";
    conf.MODE = (mode == "open_drain") ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT_PP;

    // pull
    String pull = sw["pull"] | "none";
    if (pull == "up") conf.PULL = GPIO_PULLUP;
    else if (pull == "down") conf.PULL = GPIO_PULLDOWN;
    else conf.PULL = GPIO_NOPULL;

    // speed
    String speed = sw["speed"] | "low";
    if (speed == "high") conf.SPEED = GPIO_SPEED_FREQ_HIGH;
    else if (speed == "medium") conf.SPEED = GPIO_SPEED_FREQ_MEDIUM;
    else conf.SPEED = GPIO_SPEED_FREQ_LOW;

    Serial1.println(F("LS_SW: Config Loaded"));
    Serial1.println("==== LS_SW CONFIG ====");

    Serial1.print("ENABLE: ");
    Serial1.println(conf.ENABLE);

    Serial1.print("INVERTED: ");
    Serial1.println(conf.INVERTED);

    Serial1.print("STARTUP_DELAY: ");
    Serial1.println(conf.STARTUP_DELAY);

    Serial1.print("DEFAULT_STATE: ");
    Serial1.println(conf.DEFAULT_STATE == GPIO_PIN_SET ? "ON" : "OFF");

    Serial1.print("PORT: ");
    if (conf.PORT == GPIOA) Serial1.println("GPIOA");
    else if (conf.PORT == GPIOB) Serial1.println("GPIOB");
    else if (conf.PORT == GPIOC) Serial1.println("GPIOC");
    else Serial1.println("UNKNOWN");

    Serial1.print("PIN: ");
    Serial1.println(conf.PIN);  

    Serial1.print("MODE: ");
    Serial1.println(conf.MODE == GPIO_MODE_OUTPUT_OD ? "OPEN_DRAIN" : "PUSH_PULL");

    Serial1.print("PULL: ");
    if (conf.PULL == GPIO_PULLUP) Serial1.println("PULLUP");
    else if (conf.PULL == GPIO_PULLDOWN) Serial1.println("PULLDOWN");
    else Serial1.println("NONE");

    Serial1.print("SPEED: ");
    if (conf.SPEED == GPIO_SPEED_FREQ_HIGH) Serial1.println("HIGH");
    else if (conf.SPEED == GPIO_SPEED_FREQ_MEDIUM) Serial1.println("MEDIUM");
    else Serial1.println("LOW");

    Serial1.println("======================");
    return true;
}

bool LowSideSwitch::loadConfigFromStruct(const HardwareCfg& hw)
{
    const auto& sw = hw.ls_sw;

    conf.ENABLE = sw.enable;
    conf.INVERTED = sw.inverted;
    conf.STARTUP_DELAY = sw.startup_delay_ms;

    String def = String(sw.default_state);
    conf.DEFAULT_STATE = (def == "on") ? GPIO_PIN_SET : GPIO_PIN_RESET;

    String pinStr = String(LS_SW_PIN);
    // DeviceConfig stores pin as string; fall back to macro if not provided
    // Keep existing pin (configured at compile time) so attempt to parse not required

    String mode = String(sw.mode);
    conf.MODE = (mode == "open_drain") ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT_PP;

    String pull = String(sw.pull);
    if (pull == "up") conf.PULL = GPIO_PULLUP;
    else if (pull == "down") conf.PULL = GPIO_PULLDOWN;
    else conf.PULL = GPIO_NOPULL;

    String speed = String(sw.speed);
    if (speed == "high") conf.SPEED = GPIO_SPEED_FREQ_HIGH;
    else if (speed == "medium") conf.SPEED = GPIO_SPEED_FREQ_MEDIUM;
    else conf.SPEED = GPIO_SPEED_FREQ_LOW;

    Serial1.println(F("LS_SW: Config Loaded from struct"));
    return true;
}

void LowSideSwitch::begin()
{
    if (!conf.ENABLE) return;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = conf.PIN;
    GPIO_InitStruct.Mode = conf.MODE;
    GPIO_InitStruct.Pull = conf.PULL;
    GPIO_InitStruct.Speed = conf.SPEED;

    HAL_GPIO_Init(conf.PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(conf.PORT, conf.PIN, conf.DEFAULT_STATE);

    if (conf.STARTUP_DELAY > 0) {
        HAL_Delay(conf.STARTUP_DELAY);
    }
}

void LowSideSwitch::on()
{
    if (!conf.ENABLE) return;

    GPIO_PinState state = conf.INVERTED ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(conf.PORT, conf.PIN, state);
}

void LowSideSwitch::off()
{
    if (!conf.ENABLE) return;

    GPIO_PinState state = conf.INVERTED ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(conf.PORT, conf.PIN, state);
}

void LowSideSwitch::toggle()
{
    if (!conf.ENABLE) return;
    HAL_GPIO_TogglePin(conf.PORT, conf.PIN);
}