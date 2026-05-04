#pragma once


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
    START_EXPOSURE,
    STOP_EXPOSURE
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

typedef std::vector<OutputEvent> OutputData;