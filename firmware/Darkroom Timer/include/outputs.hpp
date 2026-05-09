#pragma once


#include <cstdint>
#include <vector>


#define TESTLIGHT_OFF 0
#define TESTLIGHT_PREVIEW 1
#define TESTLIGHT_FOCUS 2

#define ANALOG_OUTPUT_FREQUENCY 2000
#define ANALOG_OUTPUT_RANGE 65535
#define ANALOG_OUTPUT_RESOLUTION 16

#define SHORT_BEEP_DURATION 100
#define LONG_BEEP_DURATION 750


enum class OutputType : uint8_t {
    SAVE_SETTINGS,
    RESET_SETTINGS,
    BUZZER_ON,
    BUZZER_OFF,
    BUZZER_BEEP,
    BUZZER_BEEP_LONG,
    BUZZER_BEEP_X,
    SSD_SET_BRIGHTNESS,
    SSD_SET_NUMBER_INT,
    SSD_SET_NUMBER_FLOAT,
    SSD_CLEAR,
    START_TIMED_EXPOSURE,
    START_INFINITE_EXPOSURE,
    STOP_EXPOSURE,
};


union OutputParameter {
    double doubleValue;
    long intValue;
    bool boolValue;
};


struct OutputEvent {
    OutputType type;
    OutputParameter parameter = {.boolValue = false};
};

struct OutputData {
    uint8_t preview_state;

    // Tone management for buzzer beeps
    uint8_t tone_counter = 0;
    bool tone_on, tone_flag;
    unsigned long tone_end_time;

    // Timer management
    unsigned long exposure_stop_time = 0;
    unsigned long exposure_start_time = 0;
    bool infinite_exposure_start, exposure_active_flag, exposure_beeper_flag;

    std::vector<OutputEvent> events;
};
