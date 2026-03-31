#include <cstdint>
#include <hardware/pio.h>
#include <hardware/irq.h>

#include "../pio/pio_rotary_encoder.pio.h"


class Encoder {
    public:
        Encoder(uint pinA);
        void begin();
        long long int getValue() { return encoder_value; };

    private:
        static void encoder_pio_irq_handler();

        static Encoder* instance;
        
        uint pinA;
        PIO pio = pio0;
        uint sm = 0;
        volatile long long int encoder_value;
};