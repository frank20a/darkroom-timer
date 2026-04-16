#include "encoder.hpp"


Encoder* Encoder::instance = nullptr;

Encoder::Encoder(uint pinA) : pinA(pinA), encoder_value(0) {
    sm = pio_claim_unused_sm(pio, true);                                        // Find first free state machine and claim it

    Encoder::instance = this;                                                   // Register this instance in the static instances array so we can refer to it from the static IRQ handler
}

void Encoder::begin() {
    // This PIO program uses a fixed .origin 0 jump table, so it must be loaded at offset 0.
    if (!pio_can_add_program_at_offset(pio, &pio_rotary_encoder_program, 0)) {
        Serial.println("Encoder PIO error: program offset 0 unavailable");
        return;
    }

    program_offset = pio_add_program_at_offset(pio, &pio_rotary_encoder_program, 0);
    pio_sm_config c = pio_rotary_encoder_program_get_default_config(program_offset);    // Get default state machine configuration struct and set up as required
    
    sm_config_set_in_pins(&c, pinA);                                           // Set the input pin for the state machine to the first encoder pin
    sm_config_set_in_shift(&c, false, false, 0);                               // Required for this ISR jump-table decoder
    sm_config_set_out_shift(&c, true, false, 0);
    pio_gpio_init(pio, pinA);                                                  // Init GPIO
    pio_gpio_init(pio, pinA + 1);
    gpio_pull_up(pinA);
    gpio_pull_up(pinA + 1);
    pio_sm_set_consecutive_pindirs(pio, sm, pinA, 2, false);                   // Set the 2 pins (A and B) as inputs
    sm_config_set_clkdiv(&c, 1250.0f);                                          // Set the speed to 100kHz
    pio_sm_init(pio, sm, program_offset + 16, &c);
    pio_sm_set_enabled(pio, sm, true);                                          // Start the state machine 
    irq_set_exclusive_handler(PIO0_IRQ_0, Encoder::encoder_pio_irq_handler);    // Set the IRQ handler for the PIO interrupt
    irq_set_enabled(PIO0_IRQ_0, true);                                          // Enable the PIO interrupt
    pio_set_irq0_source_enabled(pio, pis_interrupt0, true);                     // Enable the state machine's IRQ output to cause an interrupt on the processor
    pio_set_irq0_source_enabled(pio, pis_interrupt1, true);
}

void Encoder::encoder_pio_irq_handler() {
    if (Encoder::instance == nullptr) {
        return;
    }

    uint32_t irq_flags = pio0->irq & 0x3u;

    if (irq_flags & 0x1u) {  // IRQ 0
        Encoder::instance->encoder_value--;
    }

    if (irq_flags & 0x2u) {  // IRQ 1
        Encoder::instance->encoder_value++;
    }

    if (irq_flags != 0) {
        pio0->irq = irq_flags;  // Write-1-to-clear
    }
}