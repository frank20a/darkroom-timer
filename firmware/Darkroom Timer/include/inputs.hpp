#pragma once

#include <cstdint>
#include <vector>


enum DigitalInputs : uint8_t {
    ENCODER_BTN = 1,
    BACK_BTN,
    INPUT_2,
    INPUT_3,
    INPUT_4,
    INPUT_5,
    INPUT_6,
    INPUT_7,
    SW0_POS0,
    SW0_POS1,
    SW0_POS2,
    SW1_POS0,
    SW1_POS1,
    SW1_POS2,
    EXPOSE_BTN,
    INPUT_15
};

struct DigitalInput {
    union {
        struct {
            uint16_t enc_btn  : 1;
            uint16_t bck_btn  : 1;
            uint16_t input_2  : 1;
            uint16_t input_3  : 1;
            uint16_t input_4  : 1;
            uint16_t input_5  : 1;
            uint16_t input_6  : 1;
            uint16_t input_7  : 1;
            uint16_t sw0_pos0 : 1;
            uint16_t sw0_pos1 : 1;
            uint16_t sw0_pos2 : 1;
            uint16_t sw1_pos0 : 1;
            uint16_t sw1_pos1 : 1;
            uint16_t sw1_pos2 : 1;
            uint16_t strt_btn : 1;
            uint16_t input_15 : 1;
        };
        uint16_t inputs;
        uint8_t input_bytes[2];
    };
    
    bool operator[](int index) const {
        return (inputs >> index) & 1;
    }
};


struct AnalogInputs {
    union {
        struct {
            unsigned int analog_0;
            unsigned int analog_1;
            unsigned int analog_2;
            unsigned int analog_3;
        };
        unsigned int analog_values[4];
    };
};


struct EncoderData {
    long encoder_value;
    int encoder_delta;
};


enum EventType : uint8_t {
    ENCODER_CHANGE,
    ENCODER_INCREASE,
    ENCODER_DECREASE,
    DIGITAL_INPUT_CHANGE,
    DIGITAL_INPUT_RISE,
    DIGITAL_INPUT_FALL,
    DIGITAL_INPUT_LONG,
    SWITCH_CHANGE,
    ANALOG_INPUT_CHANGE,
    ANALOG_INPUT_INCREASE,
    ANALOG_INPUT_DECREASE
};

struct Event {
    EventType type;
    union {
        double double_value;
        long long_value;
    };
    // unsigned long timestamp;
};


struct InputData {
    DigitalInput digital_inputs;
    // AnalogInputs analog_inputs;
    EncoderData encoder_data;
    uint8_t switch_states[2];
    std::vector<Event> events;

    bool operator!=(const InputData& other) {
        return digital_inputs.inputs != other.digital_inputs.inputs ||
            encoder_data.encoder_value != other.encoder_data.encoder_value ||
            encoder_data.encoder_delta != other.encoder_data.encoder_delta ||
            switch_states[0] != other.switch_states[0] ||
            switch_states[1] != other.switch_states[1];
    }
};