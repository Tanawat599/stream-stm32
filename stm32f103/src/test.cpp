#include "mylib.h"
#include "config.h"
#include <Arduino.h>

void test(){
    Serial1.println("Testing...");
    Serial1.print("joinEUI: 0x"); Serial1.println(joinEUI, HEX);
    Serial1.print("devEUI: 0x"); Serial1.println(devEUI, HEX);
}
