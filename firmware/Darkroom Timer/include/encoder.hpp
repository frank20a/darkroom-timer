#pragma once

#include <cstdint>
#include <Arduino.h>
#include <hardware/pio.h>
#include <hardware/irq.h>
#include <hardware/gpio.h>

#include "../pio/pio_rotary_encoder.pio.h"


class Encoder {
    public:
        Encoder(uint pinA);
        void begin();
        long int getValue() { return this->encoder_value / 4; };

    private:
        static void encoder_pio_irq_handler();

        static Encoder* instance;
        
        uint pinA;
        PIO pio = pio0;
        uint sm = 0;
        uint program_offset = 0;
        volatile long encoder_value;
};