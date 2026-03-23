#ifndef MYLIB_H
#define MYLIB_H

class LoRaP2P {
public:
    void begin();
    void send();
    void receive();
};

class LoRaWan {
public:
    void begin();
    void classC();
    void classA();
};

// Test helper
void test();

#endif