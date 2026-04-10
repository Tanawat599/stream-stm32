#include <Arduino.h>
#include "mylib.h"
#include "config.h"

SDResourceManager sd(SD_MOSI, SD_MISO, SD_SCK, SD_CS);
Logger logger(&sd);

void setup() {
    sd.begin();

    logger.logMsg("JOIN", "Success");

    logger.logKV("SENSOR", 2,
        "TEMP", 28.5, "C",
        "HUM", 70.0, "%"
    );
}
void loop() {
    logger.logMixed("SENSOR", "Reading", 2,
        "TEMP", 28.5, "C",
        "HUM", 70.0, "%"
    );

    delay(5000);
}