#include "config.h"
#include <EEPROM.h>

// Volatile because we have plenty of RAM left
const Configuration configuration[]{
    // Format : {"key", default-value, min, max}

    // General
    {"LOG_INTERVAL", 10, 1, 60, 1},
    {"FILE_MAX_SIZE", 4096, 2048, 16384,3},

    // Luminosity
    {"LUMIN", 1, 0, 1, 5},
    {"LUMIN_LOW", 255, 0, 1023,7},
    {"LUMIN_HIGH", 768, 0, 1023,9},

    // Temperature
    {"TEMP_AIR", 1,0,1,11},
    {"MIN_TEMP_AIR", -10, -40, 85,13},
    {"MAX_TEMP_AIR", 60, -40, 85,15},

    // Hygrometry
    {"HYGR", 1, 0, 1,17},
    {"HYGR_MINT", 0, -40, 85,19},
    {"HYGR_MAXT", 50, -40, 85,21},

    // Pressure
    {"PRESSURE", 1, 0, 1,23},
    {"PRESSURE_MIN", 850, 300, 1100,25},
    {"PRESSURE_MAX", 1080, 300, 1100,27}
};

Config::Config() {}

void Config::init(){
    // if the eeprom was never used by our program
    if (EEPROM.read(0) != 0x42) {
        // why 42 ? because it's the answer to life, the universe and everything
        EEPROM.write(0, 0x42);
        // write the default values
        resetValues();
    }
}

int Config::getValue(String key) {
    for (auto &i : configuration) {
        if (i.key == key) {
            // retrieve the value from the eeprom
            int value;
            EEPROM.get(i.eeprom_address, value);
            return value;
        }
    }
    return -32768; // If user enters a wrong key, return -32768, min of int
}

int Config::getValue(int address) {
    for (auto &i : configuration) {
        if (i.eeprom_address == address) {
            // retrieve the value from the eeprom
            int value;
            EEPROM.get(i.eeprom_address, value);
            return value;
        }
    }
    return -32768; // If user enters a wrong key, return -32768, min of int
}

bool Config::setValue(String key, int value) {
    for (auto &i : configuration) {
        if (i.key == key) {
            // if the value is out of range
            if (value < i.min or value > i.max) {
                return false;
            } else {
                // write the value to the eeprom
                EEPROM.put(i.eeprom_address, value);
                return true;
            }
        }
    }
    return false;
}

void Config::resetValues() {
    // loop through the configuration array
    // and reset the value of the key
    for (auto &i: configuration) {
        EEPROM.put(i.eeprom_address, i.value);
    }
}

void Config::printValues() {
    // loop through the configuration array
    // and print the value of the key
    for (auto &i: configuration) {
        Serial.print(i.key);
        Serial.print(" : ");
        Serial.println(getValue(i.eeprom_address));
    }
}

int Config::getBatch(){
    int value;
    EEPROM.get(1020, value);
    return value;
}