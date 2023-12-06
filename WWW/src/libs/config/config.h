#ifndef WWW_REWORKED_CONFIG_H
#define WWW_REWORKED_CONFIG_H

#include <Arduino.h>

struct Configuration{
    String key;
    int value;
    int min;
    int max;
    int eeprom_address;
};

class Config {
public:
    Config();

    void init();
    int getValue(int address);
    int getValue(String key);
    bool setValue(String key, int value);
    void resetValues();
    void printValues();
    int getBatch();

private:
};


#endif //WWW_REWORKED_CONFIG_H
