#include "encoder.hpp"


Encoder::Encoder(uint pinA) : pinA(pinA) {
    PIO pio = pio0;                                                             // We only have one PIO, so we can select that one
    sm = pio_claim_unused_sm(pio, true);                                        // Find first free state machine and claim it

    Encoder::instance = this;                                                   // Register this instance in the static instances array so we can refer to it from the static IRQ handler
}

void Encoder::begin() {
    uint offset = pio_add_program(pio, &pio_rotary_encoder_program);            // Add program to PIO instruction memory and get offset for later reference
    pio_sm_config c = pio_rotary_encoder_program_get_default_config(offset);    // Get default state machine configuration struct and set up as required
    
    sm_config_set_in_pins(&c, pinA);                                           // Set the input pin for the state machine to the first encoder pin
    pio_gpio_init(pio, pinA);                                                  // Init GPIO
    pio_gpio_init(pio, pinA + 1);
    pio_sm_set_consecutive_pindirs(pio, sm, pinA, 2, false);                   // Set the 2 pins (A and B) as inputs
    sm_config_set_clkdiv(&c, 1250.0f);                                          // Set the speed to 100kHz
    pio_sm_init(pio, sm, offset + 16, &c);
    pio_sm_set_enabled(pio, sm, true);                                          // Start the state machine 
    irq_set_exclusive_handler(PIO0_IRQ_0, Encoder::encoder_pio_irq_handler);    // Set the IRQ handler for the PIO interrupt
    irq_set_enabled(PIO0_IRQ_0, true);                                          // Enable the PIO interrupt
    pio_set_irq0_source_enabled(pio, pis_interrupt0, true);                     // Enable the state machine's IRQ output to cause an interrupt on the processor
    pio_set_irq0_source_enabled(pio, pis_interrupt1, true);
}

void Encoder::encoder_pio_irq_handler() {
    uint32_t irq = pio_interrupt_get(pio0, 0);

    if (irq & 1) {  // IRQ 0 → CW
        Encoder::instance->encoder_value++;
        pio_interrupt_clear(pio0, 0);
    }

    if (irq & 2) {  // IRQ 1 → CCW
        Encoder::instance->encoder_value--;
        pio_interrupt_clear(pio0, 1);
    }

    pio0_hw->irq = 3;  // Clear both IRQs at once (writing a 1 to each bit clears the corresponding IRQ)
}