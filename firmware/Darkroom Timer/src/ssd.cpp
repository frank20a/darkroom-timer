#include "ssd.hpp"

void SSD::begin() {
    // Set proper pin modes and initial states
    pinMode(cs_pin, OUTPUT);
    digitalWrite(cs_pin, HIGH);  // Deselect the display

    // Set SPI settings
    spiSettings = SPISettings(5000000, MSBFIRST, SPI_MODE0); // 5 MHz, MSB first, Mode 0

    // Initialize the display (send any required initialization commands here)
    writeByte(0x09, 0xFF); // Decode mode: all digits
    writeByte(0x0A, 0x04); // Intensity: max brightness 0x00 - 0x0F
    writeByte(0x0B, 0x04); // Scan limit: display 4 digits
    writeByte(0x0C, 0x01); // Shutdown register: normal operation
    writeByte(0x0F, 0x00); // Display test: off
}

void SSD::setBrightness(uint8_t brightness) {
    if (brightness > 0x0F) {
        brightness = 0x0F; // Cap brightness to max value
    }
    writeByte(0x0A, brightness);
}

void SSD::setDigit(uint8_t digit, uint8_t value) {
    if (digit < 1 || digit > 4) {
        return; // Invalid digit index
    }
    writeByte(digit, value);
}

void SSD::setNumber(unsigned int number) {
    if (number > 9999) return; // Number too large for 4 digits

    uint8_t digits[4];
    digits[0] = number / 1000;
    digits[1] = (number / 100) % 10;
    digits[2] = (number / 10) % 10;
    digits[3] = number % 10;

    digits[2] = (digits[0] || digits[1] || digits[2]) ? digits[2] : 0xF; // Blank leading zero
    digits[1] = (digits[0] || digits[1]) ? digits[1] : 0xF; // Blank leading zero
    digits[0] = digits[0] ? digits[0] : 0xF; // Blank leading zero

    for (uint8_t i = 0; i < 4; i++) {
        setDigit(i + 1, digits[i]);
    }
}

void SSD::writeByte(uint8_t reg, uint8_t data) {
    SPI.beginTransaction(spiSettings);
    digitalWrite(cs_pin, LOW);
    SPI.transfer16(((uint16_t)reg << 8) | data);
    digitalWrite(cs_pin, HIGH);
    SPI.endTransaction();
}



