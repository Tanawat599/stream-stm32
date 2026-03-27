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

void test();

class Display {
public:
    void begin();
    void show();
};

class I2C {
public:
    void slave_begin();
    void slave_loop();
};


#endif