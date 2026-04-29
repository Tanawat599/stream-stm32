#include "mylib.h"
#include "config.h"

static bool parsePin(const String& pinStr, GPIO_TypeDef*& port, uint16_t& pin)
{
    if (pinStr.length() < 3) return false;

    char portChar = pinStr[1];
    int pinNum = pinStr.substring(2).toInt();

    switch (portChar) {
        case 'A': port = GPIOA; break;
        case 'B': port = GPIOB; break;
        case 'C': port = GPIOC; break;
        default: return false;
    }

    pin = (1 << pinNum);
    return true;
}

LowSideSwitch::LowSideSwitch()
{
    memset(&conf, 0, sizeof(conf));
}

bool LowSideSwitch::loadConfig(SDResourceManager& sd, const char* path)
{
    String json = sd.readFile(path);
    if (json == "ERROR_OPEN" || json.length() == 0) {
        Serial.println(F("LS_SW: open config failed"));
        return false;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.println(F("LS_SW: JSON parse error"));
        return false;
    }

    if (!doc["ls_sw"].is<JsonObject>()) {
        Serial.println(F("LS_SW: missing ls_sw"));
        return false;
    }

    JsonObject sw = doc["ls_sw"];

    conf.ENABLE = sw["enable"] | false;
    conf.INVERTED = sw["inverted"] | false;
    conf.STARTUP_DELAY = sw["startup_delay_ms"] | 0;

    // default state
    String def = sw["default_state"] | "off";
    conf.DEFAULT_STATE = (def == "on") ? GPIO_PIN_SET : GPIO_PIN_RESET;

    // pin
    String pinStr = sw["pin"] | "PB5";
    if (!parsePin(pinStr, conf.PORT, conf.PIN)) {
        Serial.println(F("LS_SW: invalid pin"));
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

    Serial.println(F("LS_SW: Config Loaded"));
    return true;
}

void LowSideSwitch::begin()
{
    if (!conf.ENABLE) return;

    // enable clock
    if (conf.PORT == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    if (conf.PORT == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    if (conf.PORT == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = conf.PIN;
    GPIO_InitStruct.Mode = conf.MODE;
    GPIO_InitStruct.Pull = conf.PULL;
    GPIO_InitStruct.Speed = conf.SPEED;

    HAL_GPIO_Init(conf.PORT, &GPIO_InitStruct);

    // default state
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