#include "application.hpp"
#include <SdFat.h>


static const uint8_t light_out_pins[3] = {LIGHT_OUT_R_PIN, LIGHT_OUT_G_PIN, LIGHT_OUT_B_PIN};


void print_binary8(uint8_t value) {
    for (int bit = 7; bit >= 0; --bit) {
        Serial.print((value >> bit) & 0x01);
    }
}

void print_binary16(uint16_t value) {
    for (int bit = 15; bit >= 0; --bit) {
        Serial.print((value >> bit) & 0x01);
    }
}


// Application class implementation
Application::Application() {
    epd = new EPD_Display(EPD_CS_PIN, EPD_DC_PIN, EPD_RES_PIN, EPD_BUSY_PIN, &system);
    encoder = new Encoder(ENC1_A_PIN);
    ssd = new SSD(SSD_CS_PIN);
}

// Application setup and main loop
bool Application::begin() {
    // Initialize SPI
    SPI.setRX(MISO_PIN);
    SPI.setTX(MOSI_PIN);
    SPI.setSCK(SCK_PIN);
    SPI.begin();

    // Initialize digital output pins
    pinMode(REL_ENLR_PIN, OUTPUT);
    pinMode(REL_SAFE_PIN, OUTPUT);

    // Initialize digital input shift register pins
    pinMode(DI_CLK_PIN, OUTPUT);
    pinMode(DI_PL_PIN, OUTPUT);
    pinMode(DI_IN_PIN, INPUT);
    digitalWrite(DI_PL_PIN, HIGH);

    // Initialize analog outputs
    analogWriteFreq(ANALOG_OUTPUT_FREQUENCY);
    analogWriteRange(ANALOG_OUTPUT_RANGE);
    analogWriteResolution(ANALOG_OUTPUT_RESOLUTION);
    
    // Initialize file system
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount file system, reformatting...");
        LittleFS.format();
        if (!LittleFS.begin()) {
            Serial.println("Failed to mount file system after formatting.");
            return false;
        }
    }

    // Load settings or save defaults if loading fails
    FSInfo fs_info;
    LittleFS.info(fs_info);
    if (!load_settings()) {
        Serial.println("Failed to load settings, using and saving defaults.");
        reset_settings();
    } else {
        Serial.println("Settings loaded successfully.");
    }

    // Initialize encoder
    if (!encoder->begin()) {
        Serial.println("Failed to initialize encoder.");
        return false;
    }

    // Initialize SD Card
    // if (!SD.begin(SD_CS_PIN)) {
    //     Serial.println("Failed to initialize SD card.");
    //     return false;
    // }

    // Initialize 7-segment display
    if (!ssd->begin()) {
        Serial.println("Failed to initialize 7-segment display.");
        return false;
    }
    ssd->setBrightness(system.settings.seven_segment_brightness);

    // Initialize EPD
    // Read hardware inputs once before EPD init so initial page selection
    // reflects the physical selector switch position at boot.
    read_inputs(); system.inputs.events.clear();
    if (!epd->init()) {
        Serial.println("Failed to initialize EPD.");
        return false;
    }

    set_light_output_off();

    return true;
}

void Application::mainloop_step() {
    static unsigned long last_render_time = 0;
    static short unsigned int prev_event_count = 0;
    static unsigned long int debouncer_start_time = 0;

    // Read inputs and create events before rendering to ensure the latest events are processed in the current loop iteration
    read_inputs();
    if (Serial.available())
        parse_serial_commands();

    // Debounce events to prevent excessive rendering
    if (system.inputs.events.size() != prev_event_count)
        debouncer_start_time = millis();  // Reset debouncer on new events
    prev_event_count = system.inputs.events.size();

    // Render if enough time has passed and events have stabilized
    epd->execute_logic();
    if (millis() - last_render_time >= RENDER_INTERVAL_MS && millis() - debouncer_start_time >= EVENT_DEBOUNCE_MS) {
        epd->render();
        last_render_time = millis();
    }

    // Handle outputs after rendering to ensure events are processed before the next input read
    write_outputs();
    
    // Clear events after processing to avoid stale events in the next loop iteration
    system.inputs.events.clear();  // Clear events after rendering to avoid processing stale events
}

// Input handling
void Application::read_inputs() {
    static long last_input_read_time = 0;
    long current_time = millis();
    if (current_time - last_input_read_time < INPUT_READ_INTERVAL) {
        return;
    }
    last_input_read_time = current_time;

    // Read digital inputs
    digitalWrite(DI_PL_PIN, LOW);  // Load parallel inputs into shift register
    delayMicroseconds(5);           // Short delay to ensure loading is complete
    digitalWrite(DI_PL_PIN, HIGH); // Return to shift mode
    system.inputs.digital_inputs.input_bytes[0] = shiftIn(DI_IN_PIN, DI_CLK_PIN, MSBFIRST);
    system.inputs.digital_inputs.input_bytes[1] = shiftIn(DI_IN_PIN, DI_CLK_PIN, MSBFIRST);
    // Serial.print("Digital inputs unmasked: ");
    // print_binary16(system.inputs.digital_inputs.inputs);
    // Serial.println();
    system.inputs.digital_inputs.inputs &= DIGITAL_INPUT_MASK; // Mask out unused bits
    // Serial.print("Digital inputs masked: ");
    // print_binary16(system.inputs.digital_inputs.inputs);
    // Serial.println();
    
    // Process switch states
    uint8_t tmp = std::__countr_zero((system.inputs.digital_inputs.inputs >> 9) & 0b111);  // Extract bits 8-10 for switch 0
    if (tmp < MAX_SWITCH_POSITIONS)
        system.inputs.switch_states[0] = MAX_SWITCH_POSITIONS - 1 - tmp;
    tmp = std::__countr_zero((system.inputs.digital_inputs.inputs >> 12) & 0b111);   // Extract bits 11-13 for switch 1
    if (tmp < MAX_SWITCH_POSITIONS) 
        system.inputs.switch_states[1] = MAX_SWITCH_POSITIONS - 1 - tmp;

    // Read encoder value
    long encoder_value = encoder->getValue();
    system.inputs.encoder_data.encoder_delta = encoder_value - system.inputs.encoder_data.encoder_value;
    system.inputs.encoder_data.encoder_value = encoder_value;

    create_input_events();
}

void Application::create_input_events() {
    static InputData previous_input_data;

    long current_time = millis();

    // Create events for digital input changes
    static long last_input_rise_time[16] = {0};  // Track last change time for each digital input
    for (int i = 0; i < 16; i++) {
        
        // On input change
        if (system.inputs.digital_inputs[i] != previous_input_data.digital_inputs[i]) {
            system.inputs.events.push_back({
                DIGITAL_INPUT_CHANGE, 
                .long_value = i
            });
            
            // Rising edge
            if (system.inputs.digital_inputs[i]) {  
                system.inputs.events.push_back({
                    DIGITAL_INPUT_RISE, 
                    .long_value = i
                });
                last_input_rise_time[i] = current_time;

            // Falling edge
            } else {  
                system.inputs.events.push_back({
                    DIGITAL_INPUT_FALL, 
                    .long_value = i
                });
            }
        }

        if (system.inputs.digital_inputs[i] && current_time - last_input_rise_time[i] >= LONG_PRESS_THRESH) {
            system.inputs.events.push_back({
                DIGITAL_INPUT_LONG, 
                .long_value = i
            });
            last_input_rise_time[i] = LONG_MAX;  // Reset to prevent multiple long press events
        }
    }

    // Create events for switch changes
    for (uint8_t i = 0; i < 2; i++) {
        if (system.inputs.switch_states[i] != previous_input_data.switch_states[i]) {
            system.inputs.events.push_back({
                SWITCH_CHANGE, 
                .long_value = i
            });
        }
    }

    // Create events for encoder changes
    if (system.inputs.encoder_data.encoder_delta > 0) {
        system.inputs.events.push_back({
            ENCODER_CHANGE, 
            .long_value = system.inputs.encoder_data.encoder_delta
        });
        system.inputs.events.push_back({
            ENCODER_INCREASE, 
            .long_value = system.inputs.encoder_data.encoder_value
        });
    } else if (system.inputs.encoder_data.encoder_delta < 0) {
        system.inputs.events.push_back({
            ENCODER_CHANGE, 
            .long_value = system.inputs.encoder_data.encoder_delta
        });
        system.inputs.events.push_back({
            ENCODER_DECREASE, 
            .long_value = system.inputs.encoder_data.encoder_value
        });
    }

    previous_input_data = system.inputs;
}

void Application::parse_serial_commands() {
    while (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "RSS!") {            // Reset Settings
            system.outputs.events.push_back({OutputType::RESET_SETTINGS, {}});
        } else if (cmd == "CHS!") {     // Check Settings save file
            if (load_settings()) {
                Serial.print("CHS! ");
                Serial.println(system.settings.magic_number, HEX);
            } else {
                Serial.println("ERR");
            }
        } else {                        // Unknown command
            Serial.println("ERR");
        }
    }
}

// Output handling
void Application::write_outputs() {
    handle_output_events();

    unsigned long now = millis();

    // Handle buzzer timing for non-blocking beeps
    if (system.outputs.tone_flag && now >= system.outputs.tone_end_time) {
        noTone(BUZZER_PIN);
        system.outputs.tone_on = false;
    } else if (system.outputs.tone_on) {
        tone(BUZZER_PIN, system.settings.buzzer_frequency);
        system.outputs.tone_flag = true;
        system.outputs.tone_on = false;
    }

    // Output light source
    if (system.outputs.preview_state == TESTLIGHT_FOCUS) {
        set_light_output_focus();
    
    // Light preview mode
    } else if (system.outputs.preview_state == TESTLIGHT_PREVIEW) {
        set_light_output_setpoint();
    
    // Normal timer mode
    } else {
        if (system.outputs.exposure_stop_time > now || system.outputs.infinite_exposure_start) {
            unsigned int timer_value;
            
            if (!system.outputs.exposure_active_flag)
                system.outputs.exposure_start_time = now;

            if (system.outputs.infinite_exposure_start)
                timer_value = now - system.outputs.exposure_start_time;
            else
                timer_value = system.outputs.exposure_stop_time - now;

            // Handle beeping every second
            static bool beeper_tmp_flag = false;
            if (((timer_value) / 100) % 10 == 0) {
                if (beeper_tmp_flag && system.outputs.exposure_beeper_flag) {
                    system.outputs.events.push_back({OutputType::BUZZER_BEEP, {}});
                }
            } else {
                beeper_tmp_flag = true;
            }

            // Update light output setpoint during active exposure
            set_light_output_setpoint();
            ssd->setNumber(timer_value / 1000.0, false);
            system.outputs.exposure_active_flag = true;
        } else {
            set_light_output_off();
            system.outputs.exposure_active_flag = false;
        }
    }
}

void Application::handle_output_events() {
    while (!system.outputs.events.empty()) {
        const OutputEvent& event = system.outputs.events.back();
        system.outputs.events.pop_back();

        switch (event.type) {
            case OutputType::SAVE_SETTINGS:
                save_settings();
                break;
            case OutputType::RESET_SETTINGS:
                reset_settings();
                break;
            case OutputType::BUZZER_ON:
                tone(BUZZER_PIN, system.settings.buzzer_frequency);
                break;
            case OutputType::BUZZER_OFF:
                noTone(BUZZER_PIN);
                break;
            case OutputType::BUZZER_BEEP:
                system.outputs.tone_on = true;
                system.outputs.tone_end_time = millis() + SHORT_BEEP_DURATION;
                system.outputs.tone_counter = 1;
                break;
            case OutputType::BUZZER_BEEP_LONG:
                system.outputs.tone_on = true;
                system.outputs.tone_end_time = millis() + LONG_BEEP_DURATION;
                system.outputs.tone_counter = 1;
                break;
            case OutputType::BUZZER_BEEP_X:     // WARNING: This will block the main loop, use with caution
                for (int i = 0; i < event.parameter.intValue; i++) {
                    tone(BUZZER_PIN, system.settings.buzzer_frequency);
                    delay(250);
                    noTone(BUZZER_PIN);
                    delay(100);
                }
                break;                
            case OutputType::SSD_SET_BRIGHTNESS:
                ssd->setBrightness(event.parameter.intValue);
                break;
            case OutputType::SSD_SET_NUMBER_INT:
                ssd->setNumber(event.parameter.intValue);
                break;
            case OutputType::SSD_SET_NUMBER_FLOAT:
                ssd->setNumber(event.parameter.doubleValue, false);
                break;
            case OutputType::SSD_CLEAR:
                ssd->clear();
                break;
            case OutputType::START_TIMED_EXPOSURE:
                system.outputs.exposure_stop_time = millis() + (unsigned long)(event.parameter.doubleValue * 1000);
                break;
            case OutputType::START_INFINITE_EXPOSURE:
                system.outputs.infinite_exposure_start = true;
                break;
            case OutputType::STOP_EXPOSURE:
                system.outputs.exposure_stop_time = 0;
                system.outputs.infinite_exposure_start = false;
                break;
        }
    }
}

// Output helper functions
void Application::set_light_output_focus() {
    if (system.settings.light_source == LightSourceType::LAMP) {
        digitalWrite(REL_ENLR_PIN, HIGH);
    } else if (system.settings.light_source == LightSourceType::NONE) {
        set_light_output_off();
    } else {
        for(uint8_t i = 0; i < 3; i++)
            analogWrite(light_out_pins[i], volt_to_analog_value(system.settings.analog_output_voltage_limit));
    }

    digitalWrite(REL_ENLR_PIN, LOW);
}

void Application::set_light_output_setpoint() {
    if (system.settings.light_source == LightSourceType::LAMP) {
        digitalWrite(REL_ENLR_PIN, HIGH);
    } else if (system.settings.light_source == LightSourceType::NONE) {
        set_light_output_off();
    } else  {
        for(uint8_t i = 0; i < 3; i++)
            analogWrite(light_out_pins[i], get_analog_output_value(i));
    }
}

void Application::set_light_output_off() {
    for(uint8_t i = 0; i < 3; i++)
        analogWrite(light_out_pins[i], 0);

    digitalWrite(REL_ENLR_PIN, LOW);
}

int Application::get_analog_output_value(int lamp_index) {
    int i = lamp_index;  // For readability
    double setpoint = system.timer_setpoint.color_setpoints[i];
    double voltage = 0;

    // Calculate voltage based on selected function type and coefficients
    if (system.settings.analog_output_function == AnalogOutputFunction::LINEAR) {
        voltage = map(setpoint, 
            system.settings.analog_output_range[i][0], system.settings.analog_output_range[i][1], 
            0, ANALOG_OUTPUT_RANGE
        );
    } else if (system.settings.analog_output_function == AnalogOutputFunction::POLY_3) {
        voltage = system.settings.analog_output_coeffs[i][0] * pow(setpoint, 3) + 
                system.settings.analog_output_coeffs[i][1] * pow(setpoint, 2) + 
                system.settings.analog_output_coeffs[i][2] * setpoint + 
                system.settings.analog_output_coeffs[i][3];
    } else if (system.settings.analog_output_function == AnalogOutputFunction::POLY_2) {
        voltage = system.settings.analog_output_coeffs[i][0] * pow(setpoint, 2) + 
                system.settings.analog_output_coeffs[i][1] * setpoint + 
                system.settings.analog_output_coeffs[i][2];
    } else if (system.settings.analog_output_function == AnalogOutputFunction::EXPONENTIAL) {
        voltage = system.settings.analog_output_coeffs[i][1] * pow(system.settings.analog_output_coeffs[i][0], setpoint);
    }

    // Constrain voltage to valid range
    voltage = max(min(voltage, system.settings.analog_output_voltage_limit), 0);
    
    // Serial.println("Calculated output voltage for setpoint " + String(setpoint) + " and lamp index " + String(lamp_index) + ": " + String(voltage) + "V");

    // Calculate corresponding analog output value
    return volt_to_analog_value(voltage);
}

int Application::volt_to_analog_value(double voltage) {
    return max(0, min(
        ANALOG_OUTPUT_RANGE * voltage / system.settings.analog_input_voltage, 
        ANALOG_OUTPUT_RANGE * system.settings.analog_output_voltage_limit / system.settings.analog_input_voltage
    ));
}

// Settings management
void Application::save_settings() {
    File f = LittleFS.open("/settings.bin", "w");
    f.write((const uint8_t*)&system.settings, sizeof(system.settings));
    f.close();

    set_light_output_off();

    Serial.println("Settings saved.");
    system.outputs.events.push_back({OutputType::BUZZER_BEEP, {}});
}

bool Application::load_settings() {
    File f = LittleFS.open("/settings.bin", "r");
    if (!f) return false;

    f.read((uint8_t*)&system.settings, sizeof(system.settings));
    f.close();

    set_light_output_off();

    Serial.print("Settings loaded with magic number: ");
    Serial.println(system.settings.magic_number, HEX);

    return system.settings.magic_number == 0xDEADBEEF;
}

void Application::reset_settings() {
    system.settings.light_source = LightSourceType::TRIPLE_ANALOG;
    system.settings.seven_segment_brightness = 0x01;
    system.settings.analog_output_function = AnalogOutputFunction::EXPONENTIAL;
    
    system.settings.analog_output_coeffs[0][0] = 0.993348623755158;
    system.settings.analog_output_coeffs[0][1] = 13.6209846482;
    system.settings.analog_output_coeffs[0][2] = 0;
    system.settings.analog_output_coeffs[0][3] = 0;
    system.settings.analog_output_coeffs[1][0] = 0.994356407071897;
    system.settings.analog_output_coeffs[1][1] = 13.804425449892;
    system.settings.analog_output_coeffs[1][2] = 0;
    system.settings.analog_output_coeffs[1][3] = 0;
    system.settings.analog_output_coeffs[2][0] = 0.995103379862747;
    system.settings.analog_output_coeffs[2][1] = 14.2009980159057;
    system.settings.analog_output_coeffs[2][2] = 0;
    system.settings.analog_output_coeffs[2][3] = 0;

    system.settings.analog_output_range[0][0] = 0;
    system.settings.analog_output_range[0][1] = 170;
    system.settings.analog_output_range[1][0] = 0;
    system.settings.analog_output_range[1][1] = 210;
    system.settings.analog_output_range[2][0] = 0;
    system.settings.analog_output_range[2][1] = 240;
    
    system.settings.analog_output_voltage_limit = 0;
    system.settings.analog_input_voltage = 24;
    system.settings.buzzer_frequency = 3000;

    system.settings.magic_number = 0xDEADBEEF; // Ensure magic number is set for validation

    save_settings();

    system.outputs.events.push_back({OutputType::BUZZER_BEEP_LONG, {}});
    Serial.println("Settings reset to defaults.");
}
