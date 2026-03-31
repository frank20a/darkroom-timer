/* ======================== Library includes ======================== */
#include <Arduino.h>
#include <SPI.h>
#include <cstdint>

#include "inputs.hpp"
#include "system.hpp"
#include "pin_defs.hpp"
#include "epd.hpp"
#include "encoder.hpp"


/* ======================== Constant definitions ======================== */
#define INPUT_READ_INTERVAL 25      // Minimum interval between input reads in milliseconds
#define LONG_PRESS_THRESH 500       // Threshold for long press events in milliseconds


/* ======================== Variable definitions ======================== */
InputData input_data;
SystemState system_state;
EPD_Display epd(EPD_CS_PIN, EPD_DC_PIN, EPD_RES_PIN, EPD_BUSY_PIN, &input_data, &system_state);
Encoder encoder(ENC1_A_PIN);

/* ======================== Function declarations ======================== */
void read_inputs();
void create_events();


/* ======================== Function definitions =========================== */
void setup() {
    // Initialize SPI
    SPI.setRX(MISO_PIN);
    SPI.setTX(MOSI_PIN);
    SPI.setSCK(SCK_PIN);
    SPI.begin();

    // Initialize Serial
    Serial.begin(115200);

    // Initialize digital input shift register pins
    pinMode(DI_CLK_PIN, OUTPUT);
    pinMode(DI_PL_PIN, OUTPUT);
    pinMode(DI_IN_PIN, INPUT);

    // Initialize encoder
    encoder.begin();

    // Initialize EPD
    epd.init();
}


void loop() {
    read_inputs();

    epd.render();
}


void read_inputs() {
    static long last_input_read_time = 0;
    long current_time = millis();
    if (current_time - last_input_read_time < INPUT_READ_INTERVAL) {
        return;
    }
    last_input_read_time = current_time;

    // Read digital inputs
    digitalWrite(DI_CLK_PIN, LOW);
    input_data.digital_inputs.input_bytes[0] = shiftIn(DI_IN_PIN, DI_CLK_PIN, MSBFIRST);
    input_data.digital_inputs.input_bytes[1] = shiftIn(DI_IN_PIN, DI_CLK_PIN, MSBFIRST);

    // Read encoder value
    long long int encoder_value = encoder.getValue();
    input_data.encoder_data.encoder_delta = encoder_value - input_data.encoder_data.encoder_value;
    input_data.encoder_data.encoder_value = encoder_value;

    create_events();
}

void create_events() {
    static InputData previous_input_data;

    long current_time = millis();
    
    input_data.events.clear();

    // Create events for digital input changes
    static long last_input_rise_time[16] = {0};  // Track last change time for each digital input
    for (int i = 0; i < 16; i++) {
        
        // On input change
        if (input_data.digital_inputs[i] != previous_input_data.digital_inputs[i]) {
            input_data.events.push_back({
                EventType::DIGITAL_INPUT_CHANGE, 
                .long_value = i
            });
            
            // Rising edge
            if (input_data.digital_inputs[i]) {  
                input_data.events.push_back({
                    EventType::DIGITAL_INPUT_RISE, 
                    .long_value = i
                });
                last_input_rise_time[i] = current_time;

            // Falling edge
            } else {  
                input_data.events.push_back({
                    EventType::DIGITAL_INPUT_FALL, 
                    .long_value = i
                });
            }
        }

        if (input_data.digital_inputs[i] && current_time - last_input_rise_time[i] >= LONG_PRESS_THRESH) {
            input_data.events.push_back({
                EventType::DIGITAL_INPUT_LONG, 
                .long_value = i
            });
            last_input_rise_time[i] = LONG_MAX;  // Reset to prevent multiple long press events
        }
    }

    // Create events for encoder changes
    if (input_data.encoder_data.encoder_delta > 0) {
        input_data.events.push_back({
            EventType::ENCODER_CHANGE, 
            .long_value = input_data.encoder_data.encoder_value
        });
        input_data.events.push_back({
            EventType::ENCODER_INCREASE, 
            .long_value = input_data.encoder_data.encoder_value
        });
    } else if (input_data.encoder_data.encoder_delta < 0) {
        input_data.events.push_back({
            EventType::ENCODER_CHANGE, 
            .long_value = input_data.encoder_data.encoder_value
        });
        input_data.events.push_back({
            EventType::ENCODER_DECREASE, 
            .long_value = input_data.encoder_data.encoder_delta
        });
    }

    previous_input_data = input_data;
}