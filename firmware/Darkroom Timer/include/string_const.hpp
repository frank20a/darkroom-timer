#pragma once


#define NUM_SETTINGS_MENU_ITEMS 15
#define LIGHT_SOURCE_OPTIONS 4
#define AO_FUNCTION_OPTIONS 4


namespace string_const {
    static const char* LightSources[] = {
        "None",
        "Lamp",
        "Single Analog",
        "Triple Analog"
    };

    static const char* AOFunctions[] = {
        "Linear",
        "Exponential",
        "Polynomial 2",
        "Polynomial 3"
    };

    static const char* SettingsMenuItems[] = {
        "\\/ Save Settings",
        "Light Source",
        "7SD Brightness",
        "Buzzer Frequency",
        "AO Function",
        "AO Voltage Limit",
        "AO Voltage Input",
        "AO Coeffs #1",
        "AO Coeffs #2",
        "AO Coeffs #3",
        "AO Coeffs #4",
        "AO Range LOW",
        "AO Range HIGH",
        "About",
        "/\\ Reset Settings !!!"
    };
};
