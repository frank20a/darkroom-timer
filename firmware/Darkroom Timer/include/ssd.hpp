#pragma once

#include <Arduino.h>
#include <SPI.h>

// 7 Segment Display for MAX7221
class SSD {
    public:
        SSD(uint8_t cs_pin) : cs_pin(cs_pin) {}
        bool begin();
        void setBrightness(uint8_t brightness);
        void setDigit(uint8_t digit, uint8_t value, bool decimal);
        void setDigit(uint8_t digit, uint8_t value) { setDigit(digit, value, false); }
        void setNumber(int number);
        void setNumber(long number) { setNumber((int)number); }
        void setNumber(float number, bool full_decimal);
        void setNumber(float number) { setNumber(number, false); };
        void setNumber(double number, bool full_decimal) { setNumber((float)number, full_decimal); }
        void setNumber(double number) { setNumber((float)number, false); }
        void clear();

    private:
        uint8_t cs_pin;
        SPISettings spiSettings;

        void writeByte(uint8_t reg, uint8_t data);
        void getDigits(int number, uint8_t* digits);
};