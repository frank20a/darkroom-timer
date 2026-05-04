#pragma once


#include "inputs.hpp"
#include "outputs.hpp"


enum class LightSourceType {
    NONE = 0,
    LAMP,
    SINGLE_ANALOG,
    TRIPLE_ANALOG
};

enum class AnalogOutputFunction {
    LINEAR = 0,
    EXPONENTIAL,
    POLY_2,
    POLY_3
};


struct SystemSettings {
    LightSourceType light_source = LightSourceType::NONE;
    uint8_t seven_segment_brightness = 0x0F; // 0x00 to 0x0F
    AnalogOutputFunction analog_output_function = AnalogOutputFunction::LINEAR;
    double analog_output_coeffs[3][4] = {0}; // [light_source][coeff_index]
    int analog_output_range[3][2] = {0}; // [light_source][min/max]
    double analog_output_voltage_limit = 0;
    double analog_input_voltage = 0;
    unsigned int buzzer_frequency = 3000; // In Hz
    unsigned int magic_number = 0xDEADBEEF; // For validating settings data
};

struct SystemState {
    InputData inputs;
    OutputData outputs;
    SystemSettings settings;
};