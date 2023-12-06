#include "sensors.h"
#include <Arduino.h>
#include <SdFat.h>
#include <globals.h>
#include <forcedClimate.h>
#include <Wire.h>
#include <RTClib.h>

#define LUMIN_PIN A0

int filenumber = 0;

void sensors::capture(bool sd, bool gps) {
    bool RTC_ERROR = false;
    bool GPS_ERROR = false;
    bool SENSOR_ERROR = false;
    bool SENSOR_INCOHERENT = false;
    bool SD_FULL = false;
    bool SD_ERROR = false;

    // LUMINOSITY
    int luminosity = analogRead(0);
    if (luminosity < config.getValue(7 /*LUMIN_LOW*/ )) {
        luminosity = 0;
    } else if (luminosity < config.getValue(9 /*LUMIN_HIGH*/ )) {
        luminosity = 1;
    } else {
        luminosity = 2;
    }

    // TEMPERATURE, HUMIDITY, PRESSURE
    float temperature, humidity, pressure;
    ForcedClimate bme(Wire, 0x76);
    Wire.begin();
    bme.begin();
    bme.takeForcedMeasurement();
    temperature = bme.getTemperatureCelcius();
    humidity = bme.getRelativeHumidity();
    pressure = bme.getPressure();

    // TEMPERATURE
    if (config.getValue(11 /*TEMP_AIR*/ ) == 0){ // Temperature disabled
        temperature = -32768;
    } else if (
            (temperature != -32768) and
            ((temperature < config.getValue(13 /*MIN_TEMP_AIR*/)) or
            (temperature > config.getValue(15 /*MAX_TEMP_AIR*/))))
    {
        SENSOR_INCOHERENT = true;
        temperature = -32768;
    }

    // HUMIDITY
    if (config.getValue(17 /*HYGR*/) == 0){ // Hygrometry disabled
        humidity = -32768;
    } else if (
            (humidity != -32768) and
            ((temperature < config.getValue(19 /*HYGR_MINT*/)) or
            (temperature > config.getValue(21 /*HYGR_MAXT*/))))
    {
        humidity = -32768;
    }

    // PRESSURE
    if (config.getValue(23 /*PRESSURE*/) == 0){ // Pressure disabled
        pressure = -32768;
    } else if (
            (pressure != -32768) and
            ((pressure < config.getValue(25 /*PRESSURE_MIN*/)) or
            (pressure > config.getValue(27 /*PRESSURE_MAX*/))))
    {
        SENSOR_INCOHERENT = true;
        pressure = -32768;
    }

    // GPS
    String gps_data;
    if (gps) {
        // TODO
        gps_data = "4925.7861N;00105.4226E";
    } else {
        gps_data = "NA";
    }

    // RTC
    RTC_DS1307 rtc;
    String timestamp;
    String filename_base;
    if (!rtc.begin()) {
        RTC_ERROR = true;
        timestamp = "NA";
    } else {
        DateTime now = rtc.now();
        timestamp = now.timestamp(DateTime::TIMESTAMP_FULL);
        // format of filename_base : YYMMDD
        filename_base = String(now.year() % 100) + String(now.month()) + String(now.day());
    }

    // STRING
    String csv_string = "";
    csv_string += timestamp + ";";
    if (luminosity == 0) {
        csv_string += "L;";
    } else if (luminosity == 1) {
        csv_string += "M;";
    } else {
        csv_string += "H;";
    }
    if (temperature == -32768) {
        csv_string += "NA;";
    } else {
        csv_string += String(temperature) + ";";
    }
    if (humidity == -32768) {
        csv_string += "NA;";
    } else {
        csv_string += String(humidity) + ";";
    }
    if (pressure == -32768) {
        csv_string += "NA;";
    } else {
        csv_string += String(pressure) + ";";
    }
    csv_string += gps_data;

    // SD
    if (sd and !RTC_ERROR) { //can't write to SD if RTC is not working
        SdFat sd_card;
        if (!sd_card.begin(10, SPI_HALF_SPEED)) {
            SD_ERROR = true;
        }
        if (sd_card.freeClusterCount() < 5) {
            SD_FULL = true;
        }

        if (sd_card.exists(filename_base + "_0.csv")){
            filenumber = 0;
        }

        File datafile = sd_card.open(filename_base + "_" + String(filenumber) + ".csv", FILE_WRITE);
        if (!datafile) {
            SD_ERROR = true;
        } else {
            datafile.println(csv_string);
            if (datafile.size() > config.getValue(3 /*FILE_MAX_SIZE*/)) {
                filenumber++;
            }
            datafile.close();
        }
    } else if (!sd) {
        Serial.println(csv_string);
    }

    // ERROR
    error = 0;
    if (RTC_ERROR) {
        error |= 1;
    }
    if (GPS_ERROR) {
        error |= 2;
    }
    if (SENSOR_ERROR) {
        error |= 4;
    }
    if (SENSOR_INCOHERENT) {
        error |= 8;
    }
    if (SD_FULL) {
        error |= 16;
    }
    if (SD_ERROR) {
        error |= 32;
    }
}

bool sensors::set_rtc(uint32_t timestamp) {
    RTC_DS1307 rtc;
    if (!rtc.begin()) {
        error |= 1;
        return false;
    }
    rtc.adjust(DateTime(timestamp));
    return true;
}