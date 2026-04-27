#pragma once


#include <Arduino.h>
#include <SPI.h>
// #include <SD.h>
#include <LittleFS.h>
#include <cstdint>
#include <stdio.h>
#include <bit>

#include "system.hpp"
#include "pin_defs.hpp"
#include "ssd.hpp"
#include "epd.hpp"
#include "encoder.hpp"


#define RENDER_INTERVAL_MS      333             // Minimum interval between screen renders in milliseconds
#define INPUT_READ_INTERVAL     25              // Minimum interval between input reads in milliseconds
#define LONG_PRESS_THRESH       500             // Threshold for long press events in milliseconds
#define DIGITAL_INPUT_MASK_1    0b00000110      // Valid bits for digital inputs, byte 0
#define DIGITAL_INPUT_MASK_2    0b11111110      // Valid bits for digital inputs, byte 1
#define MAX_SWITCH_POSITIONS    3               // Maximum number of positions for the switches (0, 1, 2)
#define EVENT_DEBOUNCE_MS       200             // Number of consecutive reads required to confirm an event


void print_binary8(uint8_t value);


class Application {
    public:
        Application();

        bool begin();
        void mainloop_step();

    private:
        void read_inputs();
        void write_outputs();
        void create_events();
        void parse_serial_commands();
        bool load_settings();
        void save_settings();
        void reset_settings();

        SystemState system_state;
        
        EPD_Display *epd;
        Encoder *encoder;
        SSD *ssd;
};
