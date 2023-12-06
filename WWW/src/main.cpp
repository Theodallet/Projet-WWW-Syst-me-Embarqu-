#include <Arduino.h>

#define USE_TIMER_1 true

#include <TimerInterrupt.h>

#include <ChainableLED.h>

ChainableLED led(8, 9, 1);

#define VERSION "0.0.1"
#define TimerInterval 50
#define DEBUG true
#if DEBUG
#define LogIntervalUnit 1 // When debugging, LOG_INTERVAL is in seconds
#else
#define LogIntervalUnit 60 // In production, LOG_INTERVAL is in minutes
#endif
#define ConfigTimer (30000/TimerInterval)
#define RedButtonPin 4
#define GreenButtonPin 5

volatile bool RED_LONG_FLAG = false;
volatile bool RED_SHORT_FLAG = false;
volatile bool GREEN_LONG_FLAG = false;
volatile bool GREEN_SHORT_FLAG = false;

void reset_flags() {
    RED_LONG_FLAG = false;
    RED_SHORT_FLAG = false;
    GREEN_LONG_FLAG = false;
    GREEN_SHORT_FLAG = false;
}

#include <globals.h>
#include <libs/sensors/sensors.h>

sensors sensors;
bool eco_gps_state = true;

volatile uint8_t mode = 0;
volatile uint8_t previous_mode = 0;
// 1: normal
// 2: configuration
// 3: eco
// 4: maintenance

volatile uint32_t config_timer = 0;
volatile uint32_t capture_timer = 0;

volatile uint8_t next_update = 0;
volatile bool error_state = false;
volatile uint8_t greenState = 0;
volatile uint8_t redState = 0;

void timer1_callback() {
    // CONFIGURATION
    // Here we return because in config mode,
    // there's no need to update the LED nor the buttons,
    // and doing so might interfere with Serial reading.
    if (config_timer == ConfigTimer) { led.setColorRGB(0, 255, 255, 0); }
    if (config_timer > 0) { config_timer--; }

    // CAPTURE
    if (capture_timer > 0) { capture_timer--; }

    // LED
    if (error != 0) {
        if (next_update == 0) {
            Serial.println("Updating LED");
            Serial.println(error_state);
            if (!error_state) {
                led.setColorRGB(0, 255, 0, 0);
                next_update = 1000 / TimerInterval;
            } else {
                if (error & 1) { // RTC
                    led.setColorRGB(0, 0, 0, 255);
                    next_update = 1000 / TimerInterval;
                } else if (error & 2) { // GPS
                    led.setColorRGB(0, 255, 255, 0);
                    next_update = 1000 / TimerInterval;
                } else if (error & 4) { // Sensor access error
                    led.setColorRGB(0, 0, 255, 0);
                    next_update = 1000 / TimerInterval;
                } else if (error & 8) { // Sensor returned incoherent data
                    led.setColorRGB(0, 0, 255, 0);
                    next_update = 2000 / TimerInterval;
                } else if (error & 16) { // SD full
                    led.setColorRGB(0, 255, 255, 255);
                    next_update = 1000 / TimerInterval;
                } else if (error & 32) { // SD access error
                    led.setColorRGB(0, 255, 255, 255);
                    next_update = 2000 / TimerInterval;
                }
            }
            error_state = !error_state;
        }
        next_update--;
    } else {
        next_update = 0;
        error_state = false;
        switch (mode) {
            case 0:
                led.setColorRGB(0, 255, 255, 255);
                break;
            case 1:
                led.setColorRGB(0, 0, 255, 0);
                break;
                // N.B. : config is handled above
            case 3:
                led.setColorRGB(0, 0, 0, 255);
                break;
            case 4:
                led.setColorRGB(0, 255, 60, 0);
                break;
            default:
                // should never happen !
                break;
        }
    }

    // BUTTONS
    // GREEN BUTTON
    if (digitalRead(GreenButtonPin) == LOW) {
        greenState += 1;
    } else {
        if (5000 / TimerInterval > greenState && greenState > 0) {
            GREEN_SHORT_FLAG = true;
        }
        greenState = 0;
    }
    // Why outside the if ?
    // Because we want the flag for long press to be raised as soon as 5s passed, even if it wasn't released yet.
    if (greenState == 5000 / TimerInterval) {
        GREEN_LONG_FLAG = true;
        GREEN_SHORT_FLAG = false;
    }

    // RED BUTTON
    if (digitalRead(RedButtonPin) == LOW) {
        redState += 1;
    } else {
        if (5000 / TimerInterval > redState && redState > 0) {
            RED_SHORT_FLAG = true;
        }
        redState = 0;
    }
    if (redState == 5000 / TimerInterval) {
        RED_LONG_FLAG = true;
        RED_SHORT_FLAG = false;
    }
}

void setup() {
    Serial.begin(9600);

    config.init();

    pinMode(RedButtonPin, INPUT_PULLUP);
    pinMode(GreenButtonPin, INPUT_PULLUP);

    ITimer1.init();
    ITimer1.attachInterruptInterval(TimerInterval, timer1_callback);

    mode = 0;
    int timer = 5000;
    while (timer > 0) {
        if (RED_SHORT_FLAG or RED_LONG_FLAG) {
            mode = 2;
            config_timer = ConfigTimer;
            reset_flags();
            return;
        }
        delay(1);
        timer--;
    }
    mode = 1;
}

void loop() {
    switch (mode) {
        case 1: //standard
            if (RED_LONG_FLAG) {
                previous_mode = mode;
                mode = 4;
                capture_timer = 1000 / TimerInterval;
                reset_flags();
                return;
            }
            if (GREEN_LONG_FLAG) {
                mode = 3;
                capture_timer = config.getValue(1) * LogIntervalUnit * 2000 / TimerInterval;
                reset_flags();
                return;
            }

            if (capture_timer == 0) {
                capture_timer = config.getValue(1) * LogIntervalUnit * 1000 / TimerInterval;
                Serial.println("Capturing (normal)");
                sensors.capture(true, true);
            }


        case 2: //configuration
            if (config_timer == 0) {
                mode = 1;
                capture_timer = config.getValue(1) * LogIntervalUnit * 1000 / TimerInterval;
                return;
            }
            // retrieve commands from serial
            if (Serial.available() > 0) {
                // reset the inactivity timer
                config_timer = ConfigTimer;

                String command = Serial.readStringUntil('\n');
                command.replace(" ", "");
                Serial.println();
                Serial.println(">> " + command);
                if (command == "RESET") {
                    config.resetValues();
                    Serial.println("OK");
                } else if (command == "VERSION") {
                    Serial.print("VERSION=");
                    Serial.println(VERSION);
                    Serial.print("BATCH=");
                    Serial.println(config.getBatch());
                } else if (command == "CONFIG") {
                    config.printValues();
                } else {
                    // if command contains a '='
                    if (command.indexOf('=') != -1) {
                        // split the command in two
                        String key = command.substring(0, command.indexOf('='));
                        String value = command.substring(command.indexOf('=') + 1);
                        if (key == "CLOCK"){
                            uint32_t timestamp = value.toInt();
                            if (timestamp > 0){
                                if (sensors.set_rtc(timestamp)){
                                    Serial.println("OK");
                                } else {
                                    Serial.println("ERROR");
                                }
                            } else {
                                Serial.println("ERROR");
                            }
                        }
                        if (config.setValue(key, value.toInt())) {
                            Serial.println("OK");
                        } else {
                            Serial.println("ERROR");
                        }

                    } else {
                        // if the command is a key
                        int value = config.getValue(command);
                        if (value != -32768) {
                            Serial.print(command);
                            Serial.print(" : ");
                            Serial.println(value);
                        } else {
                            Serial.println("ERROR");
                        }
                    }
                }
            }


        case 3: //eco
            if (RED_LONG_FLAG) {
                previous_mode = mode;
                mode = 4;
                reset_flags();
                return;
            }
            if (GREEN_LONG_FLAG) {
                mode = 1;
                capture_timer = config.getValue(1) * LogIntervalUnit * 1000 / TimerInterval;
                reset_flags();
                return;
            }

            if (capture_timer == 0) {
                capture_timer = config.getValue(1) * LogIntervalUnit * 2000 / TimerInterval;
                sensors.capture(true, eco_gps_state);
                eco_gps_state = !eco_gps_state;
            }

        case 4: //maintenance
            if (RED_LONG_FLAG) {
                if (previous_mode == 1){
                    mode = 1;
                    capture_timer = config.getValue(1) * LogIntervalUnit * 1000 / TimerInterval;
                } else if (previous_mode == 3){
                    mode = 3;
                    capture_timer = config.getValue(1) * LogIntervalUnit * 2000 / TimerInterval;
                }
                reset_flags();
                return;
            }

            if (capture_timer == 0) {
                capture_timer = 1000 / TimerInterval;
                sensors.capture(false, true);
            }

        default:
            // should never happen !
            break;
    }
}