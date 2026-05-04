#include "application.hpp"
#include <SdFat.h>


void print_binary8(uint8_t value) {
    for (int bit = 7; bit >= 0; --bit) {
        Serial.print((value >> bit) & 0x01);
    }
}


Application::Application() {
    epd = new EPD_Display(EPD_CS_PIN, EPD_DC_PIN, EPD_RES_PIN, EPD_BUSY_PIN, &system);
    encoder = new Encoder(ENC1_A_PIN);
    ssd = new SSD(SSD_CS_PIN);
}

bool Application::begin() {
    // Initialize digital output pins
    pinMode(REL_ENLR_PIN, OUTPUT);
    pinMode(REL_SAFE_PIN, OUTPUT);

    // Initialize digital input shift register pins
    pinMode(DI_CLK_PIN, OUTPUT);
    pinMode(DI_PL_PIN, OUTPUT);
    pinMode(DI_IN_PIN, INPUT);
    digitalWrite(DI_PL_PIN, HIGH);

    // Initialize encoder
    if (!encoder->begin()) {
        Serial.println("Failed to initialize encoder.");
        return false;
    }

    // Initialize the shift registers (digital inputs)
	// sn74165::shiftreg_init();

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

    // Initialize EPD
    if (!epd->init()) {
        Serial.println("Failed to initialize EPD.");
        return false;
    }

    // Initialize file system
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount file system, reformatting...");
        LittleFS.format();
        if (!LittleFS.begin()) {
            Serial.println("Failed to mount file system after formatting.");
            return false;
        }
    }

    FSInfo fs_info;
    LittleFS.info(fs_info);
    Serial.print("File system total bytes: ");
    Serial.println(fs_info.totalBytes);
    Serial.print("File system used bytes: ");
    Serial.println(fs_info.usedBytes);
    Serial.print("Settings file size: ");
    Serial.println(sizeof(system.settings));
    Serial.println();

    if (!load_settings()) {
        Serial.println("Failed to load settings, using and saving defaults.");
        reset_settings();
    }

    return true;
}

void Application::mainloop_step() {
    static unsigned long last_render_time = 0;
    static short unsigned int prev_event_count = 0;
    static unsigned long int debouncer_start_time = 0;

    read_inputs();
    write_outputs();
    if (Serial.available())
        parse_serial_commands();

    // Debounce events to prevent excessive rendering
    if (system.inputs.events.size() != prev_event_count)
        debouncer_start_time = millis();  // Reset debouncer on new events
    prev_event_count = system.inputs.events.size();

    // Render if enough time has passed and events have stabilized
    if (millis() - last_render_time >= RENDER_INTERVAL_MS && millis() - debouncer_start_time >= EVENT_DEBOUNCE_MS) {
        epd->execute_logic();
        epd->render();
        system.inputs.events.clear();  // Clear events after rendering to avoid processing stale events
        last_render_time = millis();
    }
}

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
    system.inputs.digital_inputs.input_bytes[0] = shiftIn(DI_IN_PIN, DI_CLK_PIN, MSBFIRST) & DIGITAL_INPUT_MASK_1;
    system.inputs.digital_inputs.input_bytes[1] = shiftIn(DI_IN_PIN, DI_CLK_PIN, MSBFIRST) & DIGITAL_INPUT_MASK_2;
    
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

    create_events();
}

void Application::create_events() {
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

void Application::write_outputs() {
    while (!system.outputs.empty()) {
        const OutputEvent& event = system.outputs.back();
        system.outputs.pop_back();

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
                tone(BUZZER_PIN, system.settings.buzzer_frequency);
                delay(250);
                noTone(BUZZER_PIN);
                break;
            case OutputType::BUZZER_BEEP_LONG:
                tone(BUZZER_PIN, system.settings.buzzer_frequency);
                delay(1000);
                noTone(BUZZER_PIN);
                break;
            case OutputType::BUZZER_BEEP_X:
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
                ssd->setNumber(event.parameter.doubleValue, true);
                break;
            case OutputType::SSD_CLEAR:
                ssd->clear();
                break;
            case OutputType::START_EXPOSURE:
                digitalWrite(REL_ENLR_PIN, HIGH);
                break;
            case OutputType::STOP_EXPOSURE:
                digitalWrite(REL_ENLR_PIN, LOW);
                break;
        }
    }
}

void Application::parse_serial_commands() {
    while (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "RSS!") {            // Reset Settings
            system.outputs.push_back({OutputType::RESET_SETTINGS, {}});
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

void Application::save_settings() {
    File f = LittleFS.open("/settings.bin", "w");
    f.write((const uint8_t*)&system.settings, sizeof(system.settings));
    f.close();

    Serial.println("Settings saved.");
    system.outputs.push_back({OutputType::BUZZER_BEEP, {}});
}

bool Application::load_settings() {
    File f = LittleFS.open("/settings.bin", "r");
    if (!f) return false;

    f.read((uint8_t*)&system.settings, sizeof(system.settings));
    f.close();

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

    system.outputs.push_back({OutputType::BUZZER_BEEP_LONG, {}});
    Serial.println("Settings reset to defaults.");
}
