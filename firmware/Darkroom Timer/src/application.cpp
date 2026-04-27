#include "application.hpp"
#include <SdFat.h>


void print_binary8(uint8_t value) {
    for (int bit = 7; bit >= 0; --bit) {
        Serial.print((value >> bit) & 0x01);
    }
}


Application::Application() {
    epd = new EPD_Display(EPD_CS_PIN, EPD_DC_PIN, EPD_RES_PIN, EPD_BUSY_PIN, &system_state);
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
    if (system_state.inputs.events.size() != prev_event_count)
        debouncer_start_time = millis();  // Reset debouncer on new events
    prev_event_count = system_state.inputs.events.size();

    // // Render if enough time has passed and events have stabilized
    if (millis() - last_render_time >= RENDER_INTERVAL_MS && millis() - debouncer_start_time >= EVENT_DEBOUNCE_MS) {
        epd->execute_logic();
        epd->render();
        system_state.inputs.events.clear();  // Clear events after rendering to avoid processing stale events
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
    system_state.inputs.digital_inputs.input_bytes[0] = shiftIn(DI_IN_PIN, DI_CLK_PIN, MSBFIRST) & DIGITAL_INPUT_MASK_1;
    system_state.inputs.digital_inputs.input_bytes[1] = shiftIn(DI_IN_PIN, DI_CLK_PIN, MSBFIRST) & DIGITAL_INPUT_MASK_2;
    
    // Process switch states
    uint8_t tmp = std::__countr_zero((system_state.inputs.digital_inputs.inputs >> 9) & 0b111);  // Extract bits 8-10 for switch 0
    if (tmp < MAX_SWITCH_POSITIONS)
        system_state.inputs.switch_states[0] = MAX_SWITCH_POSITIONS - 1 - tmp;
    tmp = std::__countr_zero((system_state.inputs.digital_inputs.inputs >> 12) & 0b111);   // Extract bits 11-13 for switch 1
    if (tmp < MAX_SWITCH_POSITIONS) 
        system_state.inputs.switch_states[1] = tmp;

    // Read encoder value
    long encoder_value = encoder->getValue();
    system_state.inputs.encoder_data.encoder_delta = encoder_value - system_state.inputs.encoder_data.encoder_value;
    system_state.inputs.encoder_data.encoder_value = encoder_value;

    create_events();
}

void Application::create_events() {
    static InputData previous_input_data;

    long current_time = millis();

    // Create events for digital input changes
    static long last_input_rise_time[16] = {0};  // Track last change time for each digital input
    for (int i = 0; i < 16; i++) {
        
        // On input change
        if (system_state.inputs.digital_inputs[i] != previous_input_data.digital_inputs[i]) {
            system_state.inputs.events.push_back({
                DIGITAL_INPUT_CHANGE, 
                .long_value = i
            });
            
            // Rising edge
            if (system_state.inputs.digital_inputs[i]) {  
                system_state.inputs.events.push_back({
                    DIGITAL_INPUT_RISE, 
                    .long_value = i
                });
                last_input_rise_time[i] = current_time;

            // Falling edge
            } else {  
                system_state.inputs.events.push_back({
                    DIGITAL_INPUT_FALL, 
                    .long_value = i
                });
            }
        }

        if (system_state.inputs.digital_inputs[i] && current_time - last_input_rise_time[i] >= LONG_PRESS_THRESH) {
            system_state.inputs.events.push_back({
                DIGITAL_INPUT_LONG, 
                .long_value = i
            });
            last_input_rise_time[i] = LONG_MAX;  // Reset to prevent multiple long press events
        }
    }

    // Create events for switch changes
    for (uint8_t i = 0; i < 2; i++) {
        if (system_state.inputs.switch_states[i] != previous_input_data.switch_states[i]) {
            system_state.inputs.events.push_back({
                SWITCH_CHANGE, 
                .long_value = i
            });
        }
    }

    // Create events for encoder changes
    if (system_state.inputs.encoder_data.encoder_delta > 0) {
        system_state.inputs.events.push_back({
            ENCODER_CHANGE, 
            .long_value = system_state.inputs.encoder_data.encoder_delta
        });
        system_state.inputs.events.push_back({
            ENCODER_INCREASE, 
            .long_value = system_state.inputs.encoder_data.encoder_value
        });
    } else if (system_state.inputs.encoder_data.encoder_delta < 0) {
        system_state.inputs.events.push_back({
            ENCODER_CHANGE, 
            .long_value = system_state.inputs.encoder_data.encoder_delta
        });
        system_state.inputs.events.push_back({
            ENCODER_DECREASE, 
            .long_value = system_state.inputs.encoder_data.encoder_value
        });
    }

    previous_input_data = system_state.inputs;
}

void Application::write_outputs() {
    ssd->setNumber(millis() / 1000.0);
}

void Application::parse_serial_commands() {
    while (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "RSS!") {            // Reset Settings
            reset_settings();

            Serial.println("RSS!");
        } else if (cmd == "CHS!") {     // Check Settings save file
            if (load_settings()) {
                Serial.print("CHS! ");
                Serial.println(system_state.settings.magic_number, HEX);
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
    f.write((const uint8_t*)&system_state.settings, sizeof(system_state.settings));
    f.close();

    Serial.println("Settings saved.");
}

bool Application::load_settings() {
    File f = LittleFS.open("/settings.bin", "r");
    if (!f) return false;

    f.read((uint8_t*)&system_state.settings, sizeof(system_state.settings));
    f.close();

    Serial.print("Settings loaded with magic number: ");
    Serial.println(system_state.settings.magic_number, HEX);

    return system_state.settings.magic_number == 0xDEADBEEF;
}

void Application::reset_settings() {
    system_state.settings.light_source = LightSourceType::TRIPLE_ANALOG;
    system_state.settings.seven_segment_brightness = 0x01;
    system_state.settings.analog_output_function = AnalogOutputFunction::EXPONENTIAL;
    
    system_state.settings.analog_output_coeffs[0][0] = 0.993348623755158;
    system_state.settings.analog_output_coeffs[0][1] = 13.6209846482;
    system_state.settings.analog_output_coeffs[0][2] = 0;
    system_state.settings.analog_output_coeffs[0][3] = 0;
    system_state.settings.analog_output_coeffs[1][0] = 0.994356407071897;
    system_state.settings.analog_output_coeffs[1][1] = 13.804425449892;
    system_state.settings.analog_output_coeffs[1][2] = 0;
    system_state.settings.analog_output_coeffs[1][3] = 0;
    system_state.settings.analog_output_coeffs[2][0] = 0.995103379862747;
    system_state.settings.analog_output_coeffs[2][1] = 14.2009980159057;
    system_state.settings.analog_output_coeffs[2][2] = 0;
    system_state.settings.analog_output_coeffs[2][3] = 0;

    system_state.settings.analog_output_range[0][0] = 0;
    system_state.settings.analog_output_range[0][1] = 170;
    system_state.settings.analog_output_range[1][0] = 0;
    system_state.settings.analog_output_range[1][1] = 210;
    system_state.settings.analog_output_range[2][0] = 0;
    system_state.settings.analog_output_range[2][1] = 240;
    
    system_state.settings.analog_output_voltage_limit = 0;
    system_state.settings.analog_input_voltage = 24;

    system_state.settings.magic_number = 0xDEADBEEF; // Ensure magic number is set for validation

    save_settings();

    Serial.println("Settings reset to defaults.");
}
