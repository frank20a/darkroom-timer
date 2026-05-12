#include "pages.hpp"
#include "epd.hpp"


unsigned int EPD_Page::counter = 0;
const epd_image_t *sidebar_icons[3] = {&Assets::clock, &Assets::calculator, &Assets::light};
std::map<PageIndex, EPD_Page*> EPD_Page::pages;


const uint8_t TimerTemplate::f_steps[FSTOP_NUM_SELECTIONS] = {12, 6, 4, 3, 2, 1};
const float TimerTemplate::t_steps[TIMER_NUM_SELECTIONS] = {0.25, 0.5, 1, 2.5, 5, 10};


void EPD_Page::render() {
    if (draw_flag || _always_draw_flag) {
        if (draw_count > FULL_REFRESH_THRESH) {
            display->setFullWindow();

            draw_count = 0;
        } else {
            display->setPartialWindow(partial_x, partial_y, partial_w, partial_h);

            draw_count++;
        }

        display->firstPage();
        do {
            _draw_header();
            draw();
        } while (display->nextPage());
    }

    draw_flag = false;
    _always_draw_flag = false;
}

void EPD_Page::execute_logic() {
    _header_logic();
    logic();
}

void EPD_Page::_draw_header() {
    // Left side bar
    display->fillRect(0, 0, 40, EPD_Type::WIDTH, GxEPD_BLACK);
    for (unsigned int i = 0; i < 3; i++) {
        bool active = (system->inputs.switch_states[1] == i);
        auto color = active ? GxEPD_WHITE : GxEPD_BLACK;

        if (active) {
            display->fillRect(2, 56 * (i + 1) , 36, 36, GxEPD_WHITE);
            display->drawBitmap((40 - sidebar_icons[i]->width)/2, 56 * (i + 1) + 2, sidebar_icons[i]->data, sidebar_icons[i]->width, sidebar_icons[i]->height, GxEPD_BLACK);
        } else {
            display->drawBitmap((40 - sidebar_icons[i]->width)/2, 56 * (i + 1) + 2, sidebar_icons[i]->data, sidebar_icons[i]->width, sidebar_icons[i]->height, GxEPD_WHITE);
        }
    }

    // Top header bar
    display->fillRect(0, 0, EPD_Type::HEIGHT, 25, GxEPD_BLACK);
    display->setTextColor(GxEPD_WHITE);
    display->setTextSize(2);
    display->setCursor(20, 5);
    display->print(get_title().c_str());
}

void EPD_Page::_header_logic() {
    if (draw_count == -1){
        on_switch_to();
        _always_draw_flag = true;
    }

   for (const auto& event : system->inputs.events) {
        if (event.type == SWITCH_CHANGE && event.long_value == 1) {
            _menu_selector();

            partial_x = 0;
            partial_y = 0;
            partial_w = 40;
            partial_h = EPD_Type::WIDTH;
            _always_draw_flag = true;

            return;
        } else if (event.long_value == BACK_BTN) {
            if (event.type == DIGITAL_INPUT_LONG) {
                switch_to_page(PageIndex::SETTINGS);
                return;
            } else if (event.type == DIGITAL_INPUT_RISE && display->get_current_page() == PageIndex::SETTINGS) {
                _menu_selector();
                return;
            }
        } 
   }
}

void EPD_Page::_menu_selector() {
    switch (system->inputs.switch_states[1]) {
        case 0:
            switch_to_page(PageIndex::TIMER);
            break;
        case 1:
            switch_to_page(PageIndex::TEST_STRIP);
            break;
        case 2:
            switch_to_page(PageIndex::LAMP_CONTROL);
            break;
    }
}


void TestPage::draw() {
    draw_knob(display, 100, 175, system->inputs.encoder_data.encoder_value, "TEST", -50, 50);
    draw_knob(display, 235, 175, system->inputs.encoder_data.encoder_value, "test", 0, 100, true);
    draw_knob(display, 360, 175, system->inputs.encoder_data.encoder_value, "TesT", 0, 200, false, 40, M_PI);
}

void TestPage::logic() {
    for (const auto& event : system->inputs.events) {
        if (event.type == ENCODER_CHANGE) {
            partial_x = 40;
            partial_y = 85;
            partial_w = EPD_Type::HEIGHT - 40;
            partial_h = EPD_Type::WIDTH - 100;
            draw_flag = true;
        }
    }
}


void TimerPage::draw() {
    const epd_image_t* timer_icons[4] = {&Assets::big_check, &Assets::big_time, &Assets::big_lamp_off, &Assets::big_lamp_on};
    const char* timer_labels[3] = {"f-stop", "normal", "toggle"};

    // Draw labels and icons for timer mode selection
    constexpr int sidebar_width = 40;
    constexpr int selector_slots = 3;
    const int selector_region_w = EPD_Type::HEIGHT - sidebar_width;
    const int slot_w = selector_region_w / selector_slots;
    const int icon_y = EPD_Type::WIDTH - 58;
    const int label_y = icon_y - 22;
    for (uint8_t i = 0; i < 3; i++) {
        const bool selected = system->inputs.switch_states[0] == i;
        const int slot_x = sidebar_width + i * slot_w;
        const epd_image_t* icon = timer_icons[i == 2 && system->outputs.exposure_active_flag ? 3 : i];

        if (selected) {
            const int bg_x = slot_x + 8;
            const int bg_y = label_y - 8;
            const int bg_w = slot_w - 16;
            const int bg_h = (icon_y + (int)icon->height + 6) - bg_y;
            display->fillRoundRect(bg_x, bg_y, bg_w, bg_h, 8, GxEPD_BLACK);
        }

        display->setTextSize(2);
        display->setTextColor(selected ? GxEPD_WHITE : GxEPD_BLACK);

        int16_t label_x1, label_y1;
        uint16_t label_w, label_h;
        display->getTextBounds(timer_labels[i], 0, 0, &label_x1, &label_y1, &label_w, &label_h);
        const int label_x = slot_x + (slot_w - (int)label_w) / 2 - label_x1;
        display->setCursor(label_x, label_y);
        display->print(timer_labels[i]);

        const int icon_x = slot_x + (slot_w - (int)icon->width) / 2;
        display->drawBitmap(icon_x, icon_y, icon->data, icon->width, icon->height, selected ? GxEPD_WHITE : GxEPD_BLACK);
    }

    // Draw text for currently selected timer mode
    const int text_x = 55;
    const int text_y = 55;
    display->setTextSize(2);
    display->setCursor(text_x, text_y);
    if (system->inputs.switch_states[0] < 2) {
        char* suffix;
        char step_buf[10];
        const char* prefix = "time step: ";
        if (system->inputs.switch_states[0] == 0){
            suffix = " stop";
            snprintf(step_buf, sizeof(step_buf), "1/%d", (int)TimerTemplate::f_steps[system->timer_setpoint.f_step_index]);
        } else if (system->inputs.switch_states[0] == 1) {
            suffix = " sec";
            snprintf(step_buf, sizeof(step_buf), "%.2f", TimerTemplate::t_steps[system->timer_setpoint.t_step_index]);
        }

        display->setTextColor(GxEPD_BLACK);
        display->print(prefix);

        int16_t x1, y1;
        uint16_t w, h;

        display->getTextBounds(prefix, 0, 0, &x1, &y1, &w, &h);
        const int step_text_x = text_x + (int)w;

        int16_t step_x1, step_y1;
        uint16_t step_w, step_h;
        display->getTextBounds(step_buf, step_text_x, text_y, &step_x1, &step_y1, &step_w, &step_h);

        if (alt_flag) {
            constexpr int pad_x = 4;
            constexpr int pad_y = 2;
            constexpr int corner_radius = 4;
            display->fillRoundRect(
                step_x1 - pad_x,
                step_y1 - pad_y,
                step_w + 2 * pad_x,
                step_h + 2 * pad_y,
                corner_radius,
                GxEPD_BLACK
            );
            display->setTextColor(GxEPD_WHITE);
        }

        display->setCursor(step_text_x, text_y);
        display->print(step_buf);

        display->setTextColor(GxEPD_BLACK);
        display->setCursor(step_text_x + (int)step_w, text_y);
        display->print(suffix);
    } else {
        display->setTextColor(GxEPD_BLACK);
        display->print("toggle lamp on/off");
    }

}

void TimerPage::logic() {
    partial_x = 40;
    partial_y = 25;
    partial_w = EPD_Type::HEIGHT - 40;  
        
    bool ssd_update_needed = false;

    for (const auto& event : system->inputs.events) {
        if (event.type == ENCODER_CHANGE) {
            if (system->inputs.switch_states[0] >= 2)
                continue;

            if (alt_flag) {
                partial_h = 60;
                draw_flag = true;

                if (system->inputs.switch_states[0] == 0)
                    system->timer_setpoint.f_step_index = max(min(system->timer_setpoint.f_step_index + event.long_value, (uint8_t)(FSTOP_NUM_SELECTIONS - 1)), (uint8_t)0);
                else if (system->inputs.switch_states[0] == 1)
                    system->timer_setpoint.t_step_index = max(min(system->timer_setpoint.t_step_index + event.long_value, (uint8_t)(TIMER_NUM_SELECTIONS - 1)), (uint8_t)0);
            } else {
                if (system->inputs.switch_states[0] == 0)
                    timer_value *= pow(2.0, (double)event.long_value / (double)TimerTemplate::f_steps[system->timer_setpoint.f_step_index]);
                else if (system->inputs.switch_states[0] == 1)
                    timer_value += event.long_value * TimerTemplate::t_steps[system->timer_setpoint.t_step_index];
                ssd_update_needed = true;
            }
        } else if (event.type == DIGITAL_INPUT_RISE && event.long_value == ENCODER_BTN) {
            partial_h = 60;
            draw_flag = true;
            
            alt_flag = !alt_flag;
        } else if (event.type == DIGITAL_INPUT_RISE && event.long_value == EXPOSE_BTN) {
            if (system->outputs.exposure_active_flag)
                system->outputs.events.push_back({OutputType::STOP_EXPOSURE, {}});
            else
                if (system->inputs.switch_states[0] < 2)
                    system->outputs.events.push_back({OutputType::START_TIMED_EXPOSURE, {.doubleValue = timer_value}});
                else 
                    system->outputs.events.push_back({OutputType::START_INFINITE_EXPOSURE, {}});

            if (system->inputs.switch_states[0] == 2) {
                partial_x = 2*EPD_Type::HEIGHT/3;
                partial_y = 150;
                partial_w = EPD_Type::HEIGHT/3;
                partial_h = EPD_Type::WIDTH - 25;
                draw_flag = true;
            }
        } else if (event.type == SWITCH_CHANGE && event.long_value == 0) {
            alt_flag = false;

            system->outputs.events.push_back({OutputType::STOP_EXPOSURE, {}});

            if (system->inputs.switch_states[0] == 2) {
                system->outputs.exposure_beeper_flag = true;
                system->outputs.events.push_back({OutputType::SSD_SET_NUMBER_INT, {.intValue = 0}});
            } else {
                system->outputs.exposure_beeper_flag = false;
                ssd_update_needed = true;
            }            
            
            partial_h = EPD_Type::WIDTH - 25;
            draw_flag = true;
        }
    }

    if (ssd_update_needed || (prev_active_flag && !system->outputs.exposure_active_flag)) {
        if (system->inputs.switch_states[0] < 2)
            system->outputs.events.push_back({OutputType::SSD_SET_NUMBER_FLOAT, {.doubleValue = timer_value}});
        else
            system->outputs.events.push_back({OutputType::SSD_SET_NUMBER_FLOAT, {.doubleValue = 0}});
    }

    prev_active_flag = system->outputs.exposure_active_flag;
}

void TimerPage::on_switch_to() {
    if (system->inputs.switch_states[0] < 2) {
        system->outputs.events.push_back({OutputType::SSD_SET_NUMBER_FLOAT, {.doubleValue = timer_value}});
    } else {
        system->outputs.events.push_back({OutputType::SSD_SET_NUMBER_FLOAT, {.doubleValue = 0}});
        system->outputs.exposure_beeper_flag = true;
    }
}

void TimerPage::on_switch_from() {
    system->outputs.events.push_back({OutputType::SSD_CLEAR, {}});
    system->outputs.exposure_beeper_flag = false; // Set beeper flag based on switch state when switching to page
}


void TestStripPage::draw() {
    const epd_image_t* timer_icons[3] = {&Assets::big_bars, &Assets::big_playpause, &Assets::big_recycle};
    const char* timer_labels[3] = {"incr.", "full", "cont."};

    // Draw labels and icons for timer mode selection
    constexpr int sidebar_width = 40;
    constexpr int selector_slots = 3;
    const int selector_region_w = EPD_Type::HEIGHT - sidebar_width;
    const int slot_w = selector_region_w / selector_slots;
    const int icon_y = EPD_Type::WIDTH - 58;
    const int label_y = icon_y - 22;
    for (uint8_t i = 0; i < 3; i++) {
        const bool selected = system->inputs.switch_states[0] == i;
        const int slot_x = sidebar_width + i * slot_w;
        const epd_image_t* icon = timer_icons[i];

        if (selected) {
            const int bg_x = slot_x + 8;
            const int bg_y = label_y - 8;
            const int bg_w = slot_w - 16;
            const int bg_h = (icon_y + (int)icon->height + 6) - bg_y;
            display->fillRoundRect(bg_x, bg_y, bg_w, bg_h, 8, GxEPD_BLACK);
        }

        display->setTextSize(2);
        display->setTextColor(selected ? GxEPD_WHITE : GxEPD_BLACK);

        int16_t label_x1, label_y1;
        uint16_t label_w, label_h;
        display->getTextBounds(timer_labels[i], 0, 0, &label_x1, &label_y1, &label_w, &label_h);
        const int label_x = slot_x + (slot_w - (int)label_w) / 2 - label_x1;
        display->setCursor(label_x, label_y);
        display->print(timer_labels[i]);

        const int icon_x = slot_x + (slot_w - (int)icon->width) / 2;
        display->drawBitmap(icon_x, icon_y, icon->data, icon->width, icon->height, selected ? GxEPD_WHITE : GxEPD_BLACK);
    }

    // Draw text for currently selected timer mode
    const int text_x = 55;
    const int text_y = 55;
    display->setTextSize(2);
    display->setCursor(text_x, text_y);
    
    char buffer[32];
    const char* suffix = " step";
    const char* prefix = "time step: ";

    display->setTextColor(GxEPD_BLACK);
    display->print(prefix);

    int16_t x1, y1;
    uint16_t w, h;

    display->getTextBounds(prefix, 0, 0, &x1, &y1, &w, &h);
    const int step_text_x = text_x + (int)w;

    snprintf(buffer, sizeof(buffer), "1/%d", (int)TimerTemplate::f_steps[system->timer_setpoint.f_step_index]);
    int16_t step_x1, step_y1;
    uint16_t step_w, step_h;
    display->getTextBounds(buffer, step_text_x, text_y, &step_x1, &step_y1, &step_w, &step_h);

    if (alt_flag) {
        constexpr int pad_x = 4;
        constexpr int pad_y = 2;
        constexpr int corner_radius = 4;
        display->fillRoundRect(
            step_x1 - pad_x,
            step_y1 - pad_y,
            step_w + 2 * pad_x,
            step_h + 2 * pad_y,
            corner_radius,
            GxEPD_BLACK
        );
        display->setTextColor(GxEPD_WHITE);
    }

    display->setCursor(step_text_x, text_y);
    display->print(buffer);

    display->setTextColor(GxEPD_BLACK);
    display->setCursor(step_text_x + (int)step_w, text_y);
    display->print(suffix);

    // Draw text for current test-strip step
    sprintf(buffer, "next test-strip step: %d", test_step + 1);
    display->setCursor(55, 85);
    display->print(buffer);
}

void TestStripPage::logic() {
    partial_x = 40;
    partial_y = 25;
    partial_h = EPD_Type::WIDTH - 25;
    partial_w = EPD_Type::HEIGHT - 40;
        
    bool ssd_update_needed = false;

    for (const auto& event : system->inputs.events) {
        if (event.type == SWITCH_CHANGE && event.long_value == 0) {
            partial_y = partial_h = EPD_Type::WIDTH / 2;
            draw_flag = true;
            test_step = 0;
            system->outputs.events.push_back({OutputType::STOP_EXPOSURE, {}});

            timer_value = base_value;
            ssd_update_needed = true;
        } else if (event.type == DIGITAL_INPUT_RISE && event.long_value == ENCODER_BTN && test_step == 0) {
            alt_flag = !alt_flag;

            partial_h = 100;
            draw_flag = true;
        } else if (event.type == ENCODER_CHANGE) {
            if (alt_flag) { 
                system->timer_setpoint.f_step_index = max(min(system->timer_setpoint.f_step_index + event.long_value, (uint8_t)(FSTOP_NUM_SELECTIONS - 1)), (uint8_t)0);
                partial_h = 60;
                draw_flag = true;
            } else if (test_step == 0) {
                base_value *= pow(2.0, (double)event.long_value / (double)TimerTemplate::f_steps[system->timer_setpoint.f_step_index]);
                timer_value = base_value;
                ssd_update_needed = true;
            }
        } else if (event.type == DIGITAL_INPUT_CHANGE && event.long_value == BACK_BTN) {
            timer_value = base_value;
            test_step = 0;
            ssd_update_needed = true;
            system->outputs.events.push_back({OutputType::STOP_EXPOSURE, {}});
        } else if (event.type == DIGITAL_INPUT_RISE && event.long_value == EXPOSE_BTN && !alt_flag) {
            if (!system->outputs.exposure_active_flag) {
                test_step++;
                if (system->inputs.switch_states[0] >= 2)
                    system->outputs.events.push_back({OutputType::START_INFINITE_EXPOSURE, {}});
                else
                    system->outputs.events.push_back({OutputType::START_TIMED_EXPOSURE, {.doubleValue = timer_value}});
            } else {
                test_step = 0;
                timer_value = base_value;
                ssd_update_needed = true;
                system->outputs.events.push_back({OutputType::STOP_EXPOSURE, {}});
            }
        }
    }

    // Handle test-strip step progression based on exposure state
    float current_value = base_value * pow(2.0, (double)test_step / (double)TimerTemplate::f_steps[system->timer_setpoint.f_step_index]);
    float prev_value = base_value * pow(2.0, (double)(test_step - 1) / (double)TimerTemplate::f_steps[system->timer_setpoint.f_step_index]);
    if (system->inputs.switch_states[0] < 2) {
        if (prev_active_flag && !system->outputs.exposure_active_flag) {
            draw_flag = true;
            switch (system->inputs.switch_states[0]) {
                case 0:
                    timer_value = current_value - prev_value;
                    ssd_update_needed = true;
                    break;
                case 1:
                    timer_value = current_value;
                    ssd_update_needed = true;
                    break;
            }
        }
    } else {
        if (millis() - system->outputs.exposure_start_time == (unsigned long)(prev_value * 1000) && !cont_buzzer_flag && system->outputs.exposure_active_flag) {
            test_step++;
            cont_buzzer_flag = true;
            
            if (current_value - prev_value >= 2.0) {
                partial_h = 60;
                draw_flag = true;
            }

            system->outputs.events.push_back({OutputType::BUZZER_BEEP, {}});
        } else {
            cont_buzzer_flag = false;
        }
    }

    // Handle SSD update when needed
    if (ssd_update_needed)
        system->outputs.events.push_back({OutputType::SSD_SET_NUMBER_FLOAT, {.doubleValue = timer_value}});


    prev_active_flag = system->outputs.exposure_active_flag;
}

void TestStripPage::on_switch_to() {
    timer_value = base_value;
    system->outputs.events.push_back({OutputType::SSD_SET_NUMBER_FLOAT, {.doubleValue = base_value}});
}

void TestStripPage::on_switch_from() {
    system->outputs.events.push_back({OutputType::SSD_CLEAR, {}});
}


void LampControlPage::draw() {
    display->setTextColor(GxEPD_BLACK);
    display->setTextSize(2);
    display->setCursor(75, 50);
    int16_t tbx, tby; uint16_t tbw, tbh;
    display->getTextBounds("Light Source", 75, 50, &tbx, &tby, &tbw, &tbh);
    display->drawLine(tbx - 5, tby + tbh + 5, tbx + tbw + 5, tby + tbh + 5, GxEPD_BLACK);
    display->print("Light Source: " + String(string_const::preview_states[system->outputs.preview_state]));

    switch (system->settings.light_source) {
        case LightSourceType::TRIPLE_ANALOG:
            for (uint8_t i = 0; i < 3; i++)
                draw_knob(display, 100 + 130 * i, 175, 
                    system->timer_setpoint.color_setpoints[i],
                    string_const::cmyk_labels[i],
                    system->settings.analog_output_range[i][0],
                    system->settings.analog_output_range[i][1],
                    system->inputs.switch_states[0] == i,
                    !system->timer_setpoint.color_enabled[i]
                );
            break;
        case LightSourceType::SINGLE_ANALOG:
            draw_knob(display, EPD_Type::HEIGHT/2 + 40, 175,
                system->timer_setpoint.color_setpoints[0],
                "Brightness",
                system->settings.analog_output_range[0][0],
                system->settings.analog_output_range[0][1],
                true,
                system->timer_setpoint.color_enabled[0]
            );
            break;
        default:
            // No light source, draw nothing
            break;
    }
}

void LampControlPage::logic() {
    partial_x = 40;
    partial_y = 20;
    partial_w = EPD_Type::HEIGHT - 40;

    for (const auto& event : system->inputs.events) {
        if (event.type == ENCODER_CHANGE) {
            if (system->settings.light_source == LightSourceType::TRIPLE_ANALOG) {
                system->timer_setpoint.color_setpoints[system->inputs.switch_states[0]] = max(min(system->timer_setpoint.color_setpoints[system->inputs.switch_states[0]] + event.long_value * 5, system->settings.analog_output_range[system->inputs.switch_states[0]][1]), system->settings.analog_output_range[system->inputs.switch_states[0]][0]);
            } else if (system->settings.light_source == LightSourceType::SINGLE_ANALOG) {
                for (uint8_t i = 0; i < 3; i++)
                    system->timer_setpoint.color_setpoints[i] = max(min(system->timer_setpoint.color_setpoints[i] + event.long_value * 5, system->settings.analog_output_range[i][1]), system->settings.analog_output_range[i][0]);
            }

            draw_flag = true; // Update display immediately to show change in setpoint
        } else if (event.type == DIGITAL_INPUT_RISE && event.long_value == EXPOSE_BTN) {
            system->outputs.preview_state = (system->outputs.preview_state + 1) % 3; // Cycle through test light states on each press
            
            draw_flag = true; // Update display immediately to show change in preview state
        } else if (event.type == DIGITAL_INPUT_RISE && event.long_value == ENCODER_BTN) {
            if (system->settings.light_source == LightSourceType::TRIPLE_ANALOG) {
                system->timer_setpoint.color_enabled[system->inputs.switch_states[0]] = !system->timer_setpoint.color_enabled[system->inputs.switch_states[0]];
            } else if (system->settings.light_source == LightSourceType::SINGLE_ANALOG) {
                for (uint8_t i = 0; i < 3; i++)
                    system->timer_setpoint.color_enabled[i] = !system->timer_setpoint.color_enabled[i];
            }

            draw_flag = true; // Update display immediately to show change in enabled state
        } else if (event.type == SWITCH_CHANGE && event.long_value == 0) {
            draw_flag = true; // Turn off preview light when switching away from lamp control page
        }
        
    }
}


void SettingsPage::draw() {
    draw_list(display, string_const::SettingsMenuItems, NUM_SETTINGS_MENU_ITEMS, selected_menu_item);
}

void SettingsPage::logic() {
    for (const auto& event : system->inputs.events) {
        if (event.type == DIGITAL_INPUT_RISE && event.long_value == ENCODER_BTN) {
            switch (selected_menu_item) {
                case OPTION_SAVE:
                    system->settings.magic_number = 0xDEADBEEF; // Ensure settings are valid
                    system->outputs.events.push_back({OutputType::SAVE_SETTINGS, {}});
                    break;
                case OPTION_FSTOP_TABLE:
                    switch_to_page(PageIndex::TABLE);
                    break;
                case OPTION_ABOUT:
                    switch_to_page(PageIndex::ABOUT);
                    break;
                case OPTION_RESET_SETTINGS:
                    system->outputs.events.push_back({OutputType::RESET_SETTINGS, {}});
                    break;
                default:
                    static_cast<SettingsValuePage*>(EPD_Page::pages[PageIndex::SETTINGS_VALUE])->set_parameters((SettingsMenuItem)selected_menu_item);
                    switch_to_page(PageIndex::SETTINGS_VALUE);
                    break;
            }
        } else if (event.type == ENCODER_CHANGE) {
            selected_menu_item += event.long_value;
            selected_menu_item = min(max(0, selected_menu_item), NUM_SETTINGS_MENU_ITEMS - 1);

            draw_flag = true; // Update display immediately to show change in selection
        }
    }
}


void SettingsValuePage::draw() {
    if (edit_mode == MULTI_SELECTION) {
        draw_list(display, multi_options, numeric_max + 1, (int)numeric_values[0]);
    } else {
        char buf[20];
        dtostrf(numeric_values[multi_index] / numeric_multiplier, 0, decimal_points, buf);
        std::string display_str = std::string(buf) + " " + std::string(unit);
        display->setTextColor(GxEPD_BLACK);
        display->setTextSize(3);
        int16_t x1, y1;
        uint16_t w, h;
        display->getTextBounds(display_str.c_str(), 0, 0, &x1, &y1, &w, &h);
        int center_x = (EPD_Type::HEIGHT - w) / 2 - x1 + 20;
        int center_y = (EPD_Type::WIDTH - h) / 2 - y1 - 35;
        display->setCursor(center_x, center_y);
        display->print(display_str.c_str());
    }

    if (edit_mode == NUMERIC_MULTI) {
        constexpr int sidebar_width = 40;
        constexpr int slot_count = 3;
        constexpr int slot_padding = 8;
        const int slot_width = (EPD_Type::HEIGHT - sidebar_width) / slot_count;
        const int label_y = EPD_Type::WIDTH - 58;
        const int value_y = EPD_Type::WIDTH - 40;

        for (uint8_t i = 0; i < 3; i++) {
            bool selected = (i == multi_index);
            const int slot_x = sidebar_width + i * slot_width;

            // Split the usable width to the right of the sidebar into even slots.
            display->setTextColor(selected ? GxEPD_WHITE : GxEPD_BLACK);
            if (selected) {
                display->fillRect(slot_x + slot_padding, label_y - 4, slot_width - (slot_padding * 2), 42, GxEPD_BLACK);
            }

            display->setTextSize(1);
            const int label_width = std::strlen(multi_labels[i]) * 6;
            display->setCursor(slot_x + (slot_width - label_width) / 2, label_y);
            display->print(multi_labels[i]);

            char buf[20];
            dtostrf(numeric_values[i] / numeric_multiplier, 0, decimal_points, buf);
            display->setTextSize(2);
            const int value_width = std::strlen(buf) * 12;
            display->setCursor(slot_x + (slot_width - value_width) / 2, value_y);
            display->print(buf);
        }
    }

    if (menu_item == SettingsPage::OPTION_BUZZER_FREQUENCY) {
        system->settings.buzzer_frequency = (unsigned int)numeric_values[0];
        system->outputs.events.push_back({OutputType::BUZZER_BEEP_LONG, {}});
    }
}

void SettingsValuePage::logic() {
    multi_index = edit_mode == NUMERIC_MULTI ? system->inputs.switch_states[0] : 0;

    for (const auto& event : system->inputs.events) {
        if (event.type == ENCODER_CHANGE) {
            numeric_values[multi_index] += event.long_value * numeric_step * numeric_multiplier;
            numeric_values[multi_index] = min(max(numeric_min * numeric_multiplier, numeric_values[multi_index]), numeric_max * numeric_multiplier);
        
            draw_flag = true; // Update display immediately to show change in value
        } else if (event.type == DIGITAL_INPUT_RISE && event.long_value == ENCODER_BTN) {
            save_value();
            if (menu_item == SettingsPage::OPTION_SEVEN_SEGMENT_BRIGHTNESS)
                system->outputs.events.push_back({OutputType::SSD_CLEAR, {}});
            switch_to_page(PageIndex::SETTINGS);
        } else if (event.type == DIGITAL_INPUT_RISE && event.long_value == BACK_BTN) {
            switch_to_page(PageIndex::SETTINGS);
        } else if (event.type == SWITCH_CHANGE && event.long_value == 0) {
            draw_flag = true; // Update display immediately to show change in selection
        }
    }

    if (draw_flag) {
        if (edit_mode == NUMERIC_SINGLE) {
            partial_x = 50;
            partial_y = 25;
            partial_w = EPD_Type::HEIGHT - 60;
            partial_h = (int)(EPD_Type::WIDTH / 3);
        } else {
            partial_x = 40;
            partial_y = 25;
            partial_w = EPD_Type::HEIGHT - 40;
            partial_h = EPD_Type::WIDTH - 25;
        }
    }
    
    if (menu_item == SettingsPage::OPTION_SEVEN_SEGMENT_BRIGHTNESS)
        system->outputs.events.push_back({OutputType::SSD_SET_BRIGHTNESS, {.intValue=(uint8_t)numeric_values[0]}});

}

void SettingsValuePage::on_switch_to() {
    unit = "";
    numeric_multiplier = 1;
    numeric_step = 1;
    decimal_points = 0;
    numeric_values[0] = 0; numeric_values[1] = 0; numeric_values[2] = 0;
    multi_options = nullptr;
    numeric_min = 0; numeric_max = 0;

    switch (menu_item) {
        case SettingsPage::OPTION_LIGHT_SOURCE:
            edit_mode = MULTI_SELECTION;
            multi_options = string_const::LightSources;
            numeric_max = LIGHT_SOURCE_OPTIONS - 1;
            numeric_values[0] = (uint8_t)system->settings.light_source;
            break;
        case SettingsPage::OPTION_SEVEN_SEGMENT_BRIGHTNESS:
            edit_mode = NUMERIC_SINGLE;
            numeric_min = 0x00;
            numeric_max = 0x0F;
            numeric_values[0] = system->settings.seven_segment_brightness;
            system->outputs.events.push_back({OutputType::SSD_SET_NUMBER_INT, {.intValue=8888}});
            break;
        case SettingsPage::OPTION_BUZZER_FREQUENCY:
            edit_mode = NUMERIC_SINGLE;
            numeric_min = 0.5;
            numeric_max = 6;
            numeric_step = 0.25;
            numeric_multiplier = 1000;
            numeric_values[0] = system->settings.buzzer_frequency;
            decimal_points = 2;
            unit = "kHz";
            break;
        case SettingsPage::OPTION_AO_FUNCTION:
            edit_mode = MULTI_SELECTION;
            multi_options = string_const::AOFunctions;
            numeric_max = AO_FUNCTION_OPTIONS - 1;
            numeric_values[0] = (uint8_t)system->settings.analog_output_function;
            break;
        case SettingsPage::OPTION_AO_VOLTAGE_LIMIT:
            edit_mode = NUMERIC_SINGLE;
            numeric_min = 1;
            numeric_max = system->settings.analog_input_voltage;
            numeric_step = 0.2;
            numeric_values[0] = system->settings.analog_output_voltage_limit;
            decimal_points = 1;
            unit = "V";
            break;
        case SettingsPage::OPTION_AO_VOLTAGE_INPUT:
            edit_mode = NUMERIC_SINGLE;
            numeric_min = 1;
            numeric_max = 60;
            numeric_step = 1;
            numeric_values[0] = system->settings.analog_input_voltage;
            decimal_points = 1;
            unit = "V";
            break;
        case SettingsPage::OPTION_AO_COEFFS_1:
            break;
        case SettingsPage::OPTION_AO_COEFFS_2:
            break;
        case SettingsPage::OPTION_AO_COEFFS_3:
            break;
        case SettingsPage::OPTION_AO_COEFFS_4:
            break;
        case SettingsPage::OPTION_AO_RANGE_LOW:
            numeric_min = -500;
            numeric_max = 500;
            numeric_step = 10;
            if (system->settings.light_source == LightSourceType::SINGLE_ANALOG) {
                edit_mode = NUMERIC_SINGLE;
                numeric_values[0] = system->settings.analog_output_range[0][0];
                system->settings.analog_output_range[1][0] = system->settings.analog_output_range[0][0];
                system->settings.analog_output_range[2][0] = system->settings.analog_output_range[0][0];
            } else if (system->settings.light_source == LightSourceType::TRIPLE_ANALOG) {
                edit_mode = NUMERIC_MULTI;
                numeric_values[0] = system->settings.analog_output_range[0][0];
                numeric_values[1] = system->settings.analog_output_range[1][0];
                numeric_values[2] = system->settings.analog_output_range[2][0];
            } else {
                edit_mode = NUMERIC_SINGLE;
                numeric_min = 0;
                numeric_max = 0;
                numeric_values[0] = 0;
            }
            break;
        case SettingsPage::OPTION_AO_RANGE_HIGH:
            numeric_min = -500;
            numeric_max = 500;
            numeric_step = 10;
            if (system->settings.light_source == LightSourceType::SINGLE_ANALOG) {
                edit_mode = NUMERIC_SINGLE;
                numeric_values[0] = system->settings.analog_output_range[0][1];
                system->settings.analog_output_range[1][1] = system->settings.analog_output_range[0][1];
                system->settings.analog_output_range[2][1] = system->settings.analog_output_range[0][1];
            } else if (system->settings.light_source == LightSourceType::TRIPLE_ANALOG) {
                edit_mode = NUMERIC_MULTI;
                numeric_values[0] = system->settings.analog_output_range[0][1];
                numeric_values[1] = system->settings.analog_output_range[1][1];
                numeric_values[2] = system->settings.analog_output_range[2][1];
            } else {
                edit_mode = NUMERIC_SINGLE;
                numeric_min = 0;
                numeric_max = 0;
                numeric_values[0] = 0;
            }
            break;
        default:
            switch_to_page(PageIndex::SETTINGS);
            break;
    }

    multi_index = edit_mode == NUMERIC_MULTI ? system->inputs.switch_states[0] : 0;
}

void SettingsValuePage::save_value() {
    switch (menu_item) {
        case SettingsPage::OPTION_LIGHT_SOURCE:
            system->settings.light_source = (LightSourceType)(int)numeric_values[0];
            if (system->settings.light_source == LightSourceType::SINGLE_ANALOG) {
                for (int i = 1; i < 3; i++) {
                    system->settings.analog_output_range[i][0] = system->settings.analog_output_range[0][0];
                    system->settings.analog_output_range[i][1] = system->settings.analog_output_range[0][1];
                    system->timer_setpoint.color_setpoints[i] = system->timer_setpoint.color_setpoints[0];
                    system->timer_setpoint.color_enabled[i] = true;
                }
            }
            break;
        case SettingsPage::OPTION_SEVEN_SEGMENT_BRIGHTNESS:
            system->settings.seven_segment_brightness = (uint8_t)numeric_values[0];
            break;
        case SettingsPage::OPTION_BUZZER_FREQUENCY:
            system->settings.buzzer_frequency = (unsigned int)numeric_values[0];
            break;
        case SettingsPage::OPTION_AO_FUNCTION:
            system->settings.analog_output_function = (AnalogOutputFunction)(int)numeric_values[0];
            break;
        case SettingsPage::OPTION_AO_VOLTAGE_LIMIT:
            system->settings.analog_output_voltage_limit = numeric_values[0];
            break;
        case SettingsPage::OPTION_AO_VOLTAGE_INPUT:
            system->settings.analog_input_voltage = numeric_values[0];
            break;
        case SettingsPage::OPTION_AO_RANGE_LOW:
            if(system->settings.light_source == LightSourceType::SINGLE_ANALOG) {
                for (int i = 0; i < 3; i++)
                    system->settings.analog_output_range[i][1] = numeric_values[0];
            } else if (system->settings.light_source == LightSourceType::TRIPLE_ANALOG) {
                for (int i = 0; i < 3; i++)
                    system->settings.analog_output_range[i][0] = numeric_values[i];
            } else {
                for (int i = 0; i < 3; i++)
                    system->settings.analog_output_range[i][0] = 0;
            }
            break;
        case SettingsPage::OPTION_AO_RANGE_HIGH:
            if(system->settings.light_source == LightSourceType::SINGLE_ANALOG) {
                for (int i = 0; i < 3; i++)
                    system->settings.analog_output_range[i][1] = numeric_values[0];
            } else if (system->settings.light_source == LightSourceType::TRIPLE_ANALOG) {
                for (int i = 0; i < 3; i++)
                    system->settings.analog_output_range[i][1] = numeric_values[i];
            } else {
                for (int i = 0; i < 3; i++)
                    system->settings.analog_output_range[i][1] = 0;
            }
            break;
    }
}


void AboutPage::draw() {
    display->setTextSize(2);
    display->setTextColor(GxEPD_BLACK);

    display->setCursor(50, 40);
    display->print("Darkroom Timer v"); display->print(PROJECT_VERSION);
    display->setCursor(50, 60);
    display->print("Created by Frank Fourlas");

    QRCodeGFX qrcode(*display);
    qrcode.setScale(3);
    qrcode.draw("https://github.com/frank20a/darkroom-timer", 150, 90);

    display->fillRect(155, 200, 250, 35, GxEPD_BLACK);
    display->setTextColor(GxEPD_WHITE);
    display->setCursor(160, 210);
    display->print("<- Back to Settings");
}

void AboutPage::logic() {
    for (const auto& event : system->inputs.events) {
        if (event.type == DIGITAL_INPUT_RISE && (event.long_value == ENCODER_BTN || event.long_value == BACK_BTN)) {
            switch_to_page(PageIndex::SETTINGS);
        }
    }
}


void TablePage::draw() {
    const int table_x = 50;
    const int table_y = 35;
    const int row_headers_count = 7;
    const int row_headers[row_headers_count] = {-3, -2, -1, 0, 1, 2, 3};

    const int col_headers_count = 7;
    const float col_headers[col_headers_count] = {0.0f, 1.0f / 6.0f, 1.0f / 4.0f, 1.0f / 3.0f, 1.0f / 2.0f, 2.0f / 3.0f, 3.0f / 4.0f};

    const int table_cols = col_headers_count + 1;
    const int table_rows = row_headers_count + 1;
    const int table_w = EPD_Type::HEIGHT - table_x - 10;
    const int table_h = EPD_Type::WIDTH - table_y - 10;
    const int cell_w = table_w / table_cols;
    const int cell_h = table_h / table_rows;

    display->setTextColor(GxEPD_BLACK);
    display->drawRect(table_x, table_y, cell_w * table_cols, cell_h * table_rows, GxEPD_BLACK);

    for (int c = 1; c < table_cols; c++) {
        const int x = table_x + c * cell_w;
        display->drawLine(x, table_y, x, table_y + cell_h * table_rows, GxEPD_BLACK);
    }
    for (int r = 1; r < table_rows; r++) {
        const int y = table_y + r * cell_h;
        display->drawLine(table_x, y, table_x + cell_w * table_cols, y, GxEPD_BLACK);
    }

    display->setTextSize(2);
    for (int c = 0; c < col_headers_count; c++) {
        int16_t x1, y1;
        uint16_t w, h;
        display->getTextBounds(string_const::table_labels[c], 0, 0, &x1, &y1, &w, &h);

        const int cell_x = table_x + (c + 1) * cell_w;
        const int cell_y = table_y;
        const int text_x = cell_x + (cell_w - (int)w) / 2 - x1;
        const int text_y = cell_y + (cell_h - (int)h) / 2 - y1;

        // Double-print with one-pixel offset as a simple bold effect.
        display->setCursor(text_x, text_y);
        display->print(string_const::table_labels[c]);
        display->setCursor(text_x + 1, text_y);
        display->print(string_const::table_labels[c]);
    }

    display->setTextSize(2);
    for (int r = 0; r < row_headers_count; r++) {
        char header_buf[6];
        snprintf(header_buf, sizeof(header_buf), "%d", row_headers[r]);

        int16_t x1, y1;
        uint16_t w, h;
        display->getTextBounds(header_buf, 0, 0, &x1, &y1, &w, &h);

        const int cell_x = table_x;
        const int cell_y = table_y + (r + 1) * cell_h;
        const int text_x = cell_x + (cell_w - (int)w) / 2 - x1;
        const int text_y = cell_y + (cell_h - (int)h) / 2 - y1;

        // Double-print with one-pixel offset as a simple bold effect.
        display->setCursor(text_x, text_y);
        display->print(header_buf);
        display->setCursor(text_x + 1, text_y);
        display->print(header_buf);
    }

    display->setTextSize(1);
    for (int r = 0; r < row_headers_count; r++) {
        for (int c = 0; c < col_headers_count; c++) {
            const float exponent = (float)row_headers[r] + col_headers[c];
            const float value = base_value * powf(2.0f, exponent);
            const float shown = diff ? (value - base_value) : value;
            const bool is_base_cell = (row_headers[r] == 0 && col_headers[c] == 0.0f);

            char value_buf[16];
            const float abs_val = fabsf(shown);
            if (abs_val >= 100.0f) {
                dtostrf(shown, 0, 0, value_buf);
            } else if (abs_val >= 10.0f) {
                dtostrf(shown, 0, 1, value_buf);
            } else {
                dtostrf(shown, 0, 2, value_buf);
            }

            int16_t x1, y1;
            uint16_t w, h;
            display->getTextBounds(value_buf, 0, 0, &x1, &y1, &w, &h);

            const int cell_x = table_x + (c + 1) * cell_w;
            const int cell_y = table_y + (r + 1) * cell_h;
            const int text_x = cell_x + (cell_w - (int)w) / 2 - x1;
            const int text_y = cell_y + (cell_h - (int)h) / 2 - y1;

            if (is_base_cell) {
                display->fillRect(cell_x + 1, cell_y + 1, cell_w - 1, cell_h - 1, GxEPD_BLACK);
                display->setTextColor(GxEPD_WHITE);
            } else {
                display->setTextColor(GxEPD_BLACK);
            }

            display->setCursor(text_x, text_y);
            display->print(value_buf);
        }
    }

    display->setTextColor(GxEPD_BLACK);
}

void TablePage::logic() {
    for (const auto& event : system->inputs.events) {
        if (event.type == DIGITAL_INPUT_RISE && event.long_value == BACK_BTN) {
            switch_to_page(PageIndex::SETTINGS);
        } else if (event.type == ENCODER_CHANGE) {
            base_value = max(1, base_value + event.long_value);

            draw_flag = true; // Update display immediately to show change in base value
        } else if (event.type == DIGITAL_INPUT_RISE && event.long_value == ENCODER_BTN) {
            diff = !diff;
            draw_flag = true; // Update display immediately to show change in diff mode
        }
    }
}


void draw_knob(EPD_Display* display, int center_x, int center_y, int value, const char* title, int min_value, int max_value, bool selected, bool off, int radius, double angle_range) {
    float _value = (value - min_value) / (float)(max_value - min_value);
    
    if (_value < 0.0 || _value > 1.0) return; // Ensure _value is between 0 and 1
    if (angle_range < M_PI || angle_range > 2 * M_PI) return; // Ensure angle range is between 180 and 360 degrees

    // Calculate start and end angles for the knob
    float start_angle = (M_PI + angle_range) / 2.0;
    float end_angle = start_angle - _value * angle_range;
    if (end_angle < 0) end_angle += 2 * M_PI;

    start_angle = M_PI * 5.0 / 4.0;

    // Create knob ring
    display->fillCircle(center_x, center_y, radius, GxEPD_BLACK);
    display->fillCircle(center_x, center_y, radius * 0.75, GxEPD_WHITE);

    // Fill the region under starting position
    if (angle_range > M_PI) {
        display->fillTriangle(
            center_x, center_y,
            center_x + radius * cos(start_angle), center_y - radius * sin(start_angle),
            center_x, center_y - radius * sin(start_angle),
            GxEPD_WHITE
        );
    }
    display->fillRect(
        center_x + radius * cos(start_angle), center_y - radius * sin(start_angle), 
        ceil(radius * abs(cos(start_angle))) + 1, ceil(radius * (1 - abs(sin(start_angle)))) + 1, 
        GxEPD_WHITE
    );

    // Draw the knob indicator
    display->drawLine(center_x, center_y, center_x + radius * cos(end_angle), center_y - radius * sin(end_angle), GxEPD_WHITE);
    if (end_angle < M_PI * 3 / 2) display->fillRect(center_x, center_y, radius + 1, radius + 1, GxEPD_WHITE);
    if (end_angle >= M_PI / 2 && end_angle < 3 * M_PI / 2) display->fillRect(center_x, center_y - radius, radius + 1, radius + 1, GxEPD_WHITE);
    if (end_angle > M_PI && end_angle <= 3 * M_PI / 2) display->fillRect(center_x - radius - 1, center_y - radius - 1, radius + 2, radius + 2, GxEPD_WHITE); 

    if (end_angle > M_PI / 2 && end_angle <= M_PI){
        display->fillTriangle(
            center_x, center_y,
            center_x + radius * cos(end_angle), center_y - radius * sin(end_angle),
            center_x, center_y - radius * sin(end_angle),
            GxEPD_WHITE
        );
        display->fillRect(
            center_x + radius * cos(end_angle), center_y - radius, 
            ceil(radius * abs(cos(end_angle))) + 1, ceil(radius * (1 - abs(sin(end_angle)))) + 1, 
            GxEPD_WHITE
        );
    } else if (end_angle <= M_PI / 2) {
        display->fillTriangle(
            center_x, center_y,
            center_x + radius * cos(end_angle), center_y - radius * sin(end_angle), 
            center_x + radius * cos(end_angle), center_y,
            GxEPD_WHITE
        );
        display->fillRect(
            center_x + radius * cos(end_angle), center_y - radius * sin(end_angle), 
            ceil(radius * (1 - abs(cos(end_angle)))) + 1, ceil(radius * abs(sin(end_angle))) + 1, 
            GxEPD_WHITE
        );
    } else if (end_angle > M_PI * 3 / 2) {
        display->fillTriangle(
            center_x, center_y,
            center_x + radius * cos(end_angle), center_y - radius * sin(end_angle), 
            center_x, center_y - radius * sin(end_angle),
            GxEPD_WHITE
        );
        display->fillRect(
            center_x, center_y - radius * sin(end_angle), 
            ceil(radius * abs(cos(end_angle))) + 1, ceil(radius * ( 1 - abs(sin(end_angle)))) + 1, 
            GxEPD_WHITE
        );
    } else {
        display->fillTriangle(
            center_x, center_y,
            center_x + radius * cos(end_angle), center_y - radius * sin(end_angle), 
            center_x + radius * cos(end_angle), center_y,
            GxEPD_WHITE
        );
        display->fillRect(
            center_x - radius, center_y,
            ceil(radius * (1 - abs(cos(end_angle)))) + 1, ceil(radius * abs(sin(end_angle))) + 1,
            GxEPD_WHITE
        );
    }

    // Draw the knob center and indicator
    display->fillCircle(center_x, center_y, radius * 0.1, GxEPD_BLACK);
    display->drawLine(center_x, center_y, center_x + radius * 0.65 * cos(end_angle), center_y - radius * 0.65 * sin(end_angle), GxEPD_BLACK);
    display->drawLine(center_x, center_y, center_x + radius * 0.65 * cos(end_angle + 0.02), center_y - radius * 0.65 * sin(end_angle + 0.02), GxEPD_BLACK);
    display->drawLine(center_x, center_y, center_x + radius * 0.65 * cos(end_angle - 0.02), center_y - radius * 0.65 * sin(end_angle - 0.02), GxEPD_BLACK);

    // Print value
    char value_buf[16];
    if (off)
        sprintf(value_buf, "OFF");
    else
        dtostrf(value, 0, 0, value_buf);

    display->setTextSize(2);
    int16_t value_x1, value_y1;
    uint16_t value_w, value_h;
    display->getTextBounds(value_buf, 0, 0, &value_x1, &value_y1, &value_w, &value_h);

    const int value_center_y = center_y + (int)(radius);
    const int value_text_x = center_x - ((int)value_w / 2) - value_x1;
    const int value_text_y = value_center_y - ((int)value_h / 2) - value_y1;

    display->setTextColor(GxEPD_BLACK);
    display->setCursor(value_text_x, value_text_y);
    display->print(value_buf);
    
    // Print title
    int16_t tbx, tby; uint16_t tbw, tbh;
    display->getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
    if (!selected) {
        display->setTextColor(GxEPD_BLACK);
    } else {
        display->setTextColor(GxEPD_WHITE);
        display->fillRoundRect(center_x - tbw / 2 - 5, center_y - radius * 1.15 - tbh - 5, tbw + 10, tbh + 10, 5, GxEPD_BLACK);
    }
    display->setCursor(center_x - tbw / 2, center_y - radius * 1.15 - tbh);
    display->print(title);
}

void draw_scrollbar(EPD_Display* display, int val, int max_val) {
    const int visible_items = 6;
    const int bar_x = EPD_Type::HEIGHT - 20;
    const int bar_y = 45;
    const int bar_w = 10;
    const int bar_h = EPD_Type::WIDTH - 70;
    const int inner_padding = 1;

    // display->fillRect(bar_x, bar_y, bar_w, bar_h, GxEPD_WHITE);
    display->drawRect(bar_x, bar_y, bar_w, bar_h, GxEPD_BLACK);

    if (max_val <= 0) {
        return;
    }

    const int inner_x = bar_x + inner_padding;
    const int inner_y = bar_y + inner_padding;
    const int inner_w = bar_w - 2 * inner_padding;
    const int inner_h = bar_h - 2 * inner_padding;

    int thumb_y = inner_y;
    int thumb_h = inner_h;

    if (max_val > visible_items) {
        thumb_h = max(1, (inner_h * visible_items) / max_val);

        const int window_start = min(max(0, val - 3), max_val - visible_items);
        const int max_window_start = max_val - visible_items;
        const int travel = inner_h - thumb_h;

        thumb_y = inner_y + (window_start * travel) / max_window_start;
    }

    display->fillRect(inner_x, thumb_y, inner_w, thumb_h, GxEPD_BLACK);
}

void draw_list(EPD_Display* display, const char** items, int item_count, int selected_index) {
    int start = max(0, min(selected_index - 3, item_count - 6));

    for (int i = start; i < min(start + 6, item_count); i++) {
        bool selected = (i == selected_index);

        display->setTextColor(selected ? GxEPD_WHITE : GxEPD_BLACK);
        if (selected)
            display->fillRect(50, 35 + (i - start) * 30, EPD_Type::HEIGHT - 100, 35, GxEPD_BLACK);
        display->setTextSize(2);
        display->setCursor(60, 45 + (i - start) * 30);
        display->print(items[i]);
    }

    draw_scrollbar(display, selected_index, item_count);
}
