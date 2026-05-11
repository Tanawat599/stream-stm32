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
    String pinStr = "PB5";
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

    Serial1.print(F("[LS_SW] Enabled: "));
    Serial1.print(conf.ENABLE ? "1" : "0");
    Serial1.print(F(", Inverted: "));
    Serial1.print(conf.INVERTED ? "1" : "0");
    Serial1.print(F(", Default: "));
    Serial1.print(conf.DEFAULT_STATE == GPIO_PIN_SET ? "ON" : "OFF");
    Serial1.print(F(", Mode: "));
    Serial1.print(conf.MODE == GPIO_MODE_OUTPUT_OD ? "OPEN_DRAIN" : "PUSH_PULL");
    Serial1.print(F(", Pull: "));
    if (conf.PULL == GPIO_PULLUP) Serial1.print("UP");
    else if (conf.PULL == GPIO_PULLDOWN) Serial1.print("DOWN");
    else Serial1.print("NONE");
    Serial1.print(F(", Speed: "));
    if (conf.SPEED == GPIO_SPEED_FREQ_HIGH) Serial1.print("HIGH");
    else if (conf.SPEED == GPIO_SPEED_FREQ_MEDIUM) Serial1.print("MEDIUM");
    else Serial1.print("LOW");
    Serial1.print(F(", Delay: "));
    Serial1.print(conf.STARTUP_DELAY);
    Serial1.println(" ms");
    return true;
}

bool LowSideSwitch::loadConfigFromStruct(const HardwareCfg& hw) {
    const auto& sw = hw.ls_sw;

    conf.ENABLE = sw.enable;
    conf.INVERTED = sw.inverted;
    conf.STARTUP_DELAY = sw.startup_delay_ms;

    if (strcmp(sw.default_state, "on") == 0) 
        conf.DEFAULT_STATE = GPIO_PIN_SET;
    else 
        conf.DEFAULT_STATE = GPIO_PIN_RESET;

    if (strcmp(sw.mode, "open_drain") == 0) 
        conf.MODE = GPIO_MODE_OUTPUT_OD;
    else 
        conf.MODE = GPIO_MODE_OUTPUT_PP;

    if (strcmp(sw.pull, "up") == 0) 
        conf.PULL = GPIO_PULLUP;
    else if (strcmp(sw.pull, "down") == 0) 
        conf.PULL = GPIO_PULLDOWN;
    else 
        conf.PULL = GPIO_NOPULL;

    if (strcmp(sw.speed, "high") == 0) 
        conf.SPEED = GPIO_SPEED_FREQ_HIGH;
    else if (strcmp(sw.speed, "medium") == 0) 
        conf.SPEED = GPIO_SPEED_FREQ_MEDIUM;
    else 
        conf.SPEED = GPIO_SPEED_FREQ_LOW;

    conf.PIN  = LS_SW_PIN;
    conf.PORT = LS_SW_PORT;

    Serial1.print(F("[LS_SW] Enabled: "));
    Serial1.print(conf.ENABLE ? "1" : "0");
    Serial1.print(F(", Inverted: "));
    Serial1.print(conf.INVERTED ? "1" : "0");
    Serial1.print(F(", Default: "));
    Serial1.print(conf.DEFAULT_STATE == GPIO_PIN_SET ? "ON" : "OFF");
    Serial1.print(F(", Mode: "));
    Serial1.print(conf.MODE == GPIO_MODE_OUTPUT_OD ? "OPEN_DRAIN" : "PUSH_PULL");
    Serial1.print(F(", Pull: "));
    if (conf.PULL == GPIO_PULLUP) Serial1.print("UP");
    else if (conf.PULL == GPIO_PULLDOWN) Serial1.print("DOWN");
    else Serial1.print("NONE");
    Serial1.print(F(", Speed: "));
    if (conf.SPEED == GPIO_SPEED_FREQ_HIGH) Serial1.print("HIGH");
    else if (conf.SPEED == GPIO_SPEED_FREQ_MEDIUM) Serial1.print("MEDIUM");
    else Serial1.print("LOW");
    Serial1.print(F(", Delay: "));
    Serial1.print(conf.STARTUP_DELAY);
    Serial1.println(" ms");
    return true;
}
void LowSideSwitch::begin()
{
    if (!conf.ENABLE) return;

    if (conf.STARTUP_DELAY > 10000) {
        Serial1.printf("[LS_SW] WARNING: STARTUP_DELAY=%lu ms too high, reducing to 5000 ms\n", conf.STARTUP_DELAY);
        conf.STARTUP_DELAY = 5000;
    }

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