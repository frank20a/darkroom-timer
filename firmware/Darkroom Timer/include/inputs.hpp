#pragma once

#include <cstdint>
#include <vector>


typedef struct {
    union {
        struct {
            uint16_t enc_btn  : 1;
            uint16_t input_1  : 1;
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
    };
    
    bool operator[](int index) const {
        return (inputs >> index) & 1;
    }
} DigitalInput;


typedef struct {
    union {
        struct {
            unsigned int analog_0;
            unsigned int analog_1;
            unsigned int analog_2;
            unsigned int analog_3;
        };
        unsigned int analog_values[4];
    };
} AnalogInput;


typedef struct {
    long int encoder_value;
    int encoder_delta;
} EncoderData;


enum EventType : unsigned int {
    ENCODER_CHANGE,
    ENCODER_INCREASE,
    ENCODER_DECREASE,
    DIGITAL_INPUT_CHANGE,
    DIGITAL_INPUT_RISE,
    DIGITAL_INPUT_FALL,
    DIGITAL_INPUT_LONG,
    ANALOG_INPUT_CHANGE,
    ANALOG_INPUT_INCREASE,
    ANALOG_INPUT_DECREASE
};

typedef struct {
    EventType type;
    union {
        double double_value;
        long int long_value;
    };
} Event;


typedef struct {
    DigitalInput digital_inputs;
    AnalogInput analog_inputs;
    EncoderData encoder_data;
    std::vector<Event> events;
} InputData;