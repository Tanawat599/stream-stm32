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

class Commu {
public:
    void begin();
    void i2c_slave();
    void i2c_master();
    void rs485_send();
    void rs485_receive();
    void spi_master();
}


#endif