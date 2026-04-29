#include <Arduino.h>
#include "mylib.h"
#include "config.h"

LowSideSwitch ls;

void setup() {
    SDResourceManager sd(SD_MOSI, SD_MISO, SD_SCK, SD_CS);

    if (ls.loadConfig(sd, "/config.json")) {
        ls.begin();
    }
}

void loop() {
    ls.on();
    delay(1000);

    ls.off();
    delay(1000);
}