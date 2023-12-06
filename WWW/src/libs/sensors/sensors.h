#ifndef WWW_REWORKED_SENSORS_H
#define WWW_REWORKED_SENSORS_H

#include "Arduino.h"

class sensors {
public:
    void capture(bool sd, bool gps);
    bool set_rtc(uint32_t timestamp);
};


#endif //WWW_REWORKED_SENSORS_H
