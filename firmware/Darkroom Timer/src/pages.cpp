#include "pages.hpp"
#include "epd.hpp"


unsigned int EPD_Page::counter = 0;
const epd_image_t *sidebar_icons[3] = {&Assets::light, &Assets::calculator, &Assets::gear};
std::map<PageIndex, EPD_Page*> EPD_Page::pages;


void EPD_Page::render() {
    if (_always_draw_condition() || draw_condition()) {
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

bool EPD_Page::_always_draw_condition() {
    if (draw_count == -1){
        on_switch_to();
        return true;
    }

    for (const auto& event : system->inputs.events) {
        if (event.type == SWITCH_CHANGE && event.long_value == 1) {
            partial_x = 0;
            partial_y = 0;
            partial_w = 40;
            partial_h = EPD_Type::WIDTH;
            return true;
        }
    }
    return false;
}

void EPD_Page::_header_logic() {
   for (const auto& event : system->inputs.events) {
        if (event.type == SWITCH_CHANGE && event.long_value == 1) {
            switch (system->inputs.switch_states[1]) {
                case 0:
                    switch_to_page(PageIndex::TEST);
                    break;
                case 1:
                    switch_to_page(PageIndex::TEST);
                    break;
                case 2:

                    switch_to_page(PageIndex::SETTINGS);
                    break;
            }
        }
   }
}


void TestPage::draw() {
    draw_knob(display, 156, 200, (double)(system->inputs.encoder_data.encoder_value / 100.0), "Cyan");
    draw_knob(display, 260, 200, (double)(system->inputs.encoder_data.encoder_value / 100.0), "Magenta", true);
    draw_knob(display, 364, 200, (double)(system->inputs.encoder_data.encoder_value / 100.0), "Yellow");
}

bool TestPage::draw_condition() {
    for (const auto& event : system->inputs.events) {
        if (event.type == ENCODER_CHANGE) {
            partial_x = 115;
            partial_y = 170;
            partial_w = 300;
            partial_h = 80;
            return true;
        }
    }
    return false;
}


void SettingsPage::draw() {
    // int start = min(max(0, selected_menu_item - 3), NUM_SETTINGS_MENU_ITEMS - 6);
    // for (int i = start; i < start + 6; i++) {

    //     bool selected = (i == selected_menu_item);

    //     display->setTextColor(selected ? GxEPD_WHITE : GxEPD_BLACK);
    //     if (selected)
    //         display->fillRect(50, 35 + (i - start) * 30, EPD_Type::HEIGHT - 100, 35, GxEPD_BLACK);
    //     display->setTextSize(2);
    //     display->setCursor(60, 45 + (i - start) * 30);
    //     display->print(string_const::SettingsMenuItems[i]);
    // }

    // draw_scrollbar(display, selected_menu_item, NUM_SETTINGS_MENU_ITEMS);

    draw_list(display, string_const::SettingsMenuItems, NUM_SETTINGS_MENU_ITEMS, selected_menu_item);
}

bool SettingsPage::draw_condition() {
    bool ret = false;

    for (const auto& event : system->inputs.events) {
        if (event.type == ENCODER_CHANGE) {
            ret = true;
            break;
        }
    }

    if (ret) {
        partial_x = 50;
        partial_y = 35;
        partial_w = EPD_Type::HEIGHT - 55;
        partial_h = 180;
    }

    return ret;
}

void SettingsPage::logic() {
    for (const auto& event : system->inputs.events) {
        if (event.type == DIGITAL_INPUT_RISE && event.long_value == ENCODER_BTN) {
            switch (selected_menu_item) {
                case OPTION_SAVE:
                    system->settings.magic_number = 0xDEADBEEF; // Ensure settings are valid
                    system->outputs.push_back({OutputType::SAVE_SETTINGS, {}});
                    break;
                case OPTION_ABOUT:
                    switch_to_page(PageIndex::ABOUT);
                    break;
                case OPTION_RESET_SETTINGS:
                    system->outputs.push_back({OutputType::RESET_SETTINGS, {}});
                    break;
                default:
                    static_cast<SettingsValuePage*>(EPD_Page::pages[PageIndex::SETTINGS_VALUE])->set_parameters((SettingsMenuItem)selected_menu_item);
                    switch_to_page(PageIndex::SETTINGS_VALUE);
                    break;
            }
        } else if (event.type == ENCODER_CHANGE) {
            selected_menu_item += event.long_value;
            selected_menu_item = min(max(0, selected_menu_item), NUM_SETTINGS_MENU_ITEMS - 1);
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
        system->outputs.push_back({OutputType::BUZZER_BEEP, {}});
    }
}

bool SettingsValuePage::draw_condition() {
    bool ret = false;

    for (const auto& event : system->inputs.events) {
        if (event.type == ENCODER_CHANGE || (event.type == SWITCH_CHANGE && event.long_value == 0)) {
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

            return true;
        }
    }

    return false;
}

void SettingsValuePage::logic() {
    multi_index = edit_mode == NUMERIC_MULTI ? system->inputs.switch_states[0] : 0;

    for (const auto& event : system->inputs.events) {
        if (event.type == ENCODER_CHANGE) {
            numeric_values[multi_index] += event.long_value * numeric_step * numeric_multiplier;
            numeric_values[multi_index] = min(max(numeric_min * numeric_multiplier, numeric_values[multi_index]), numeric_max * numeric_multiplier);
        } else if (event.type == DIGITAL_INPUT_RISE && event.long_value == ENCODER_BTN) {
            save_value();
            if (menu_item == SettingsPage::OPTION_SEVEN_SEGMENT_BRIGHTNESS)
                system->outputs.push_back({OutputType::SSD_CLEAR, {}});
            switch_to_page(PageIndex::SETTINGS);
        }
    }
    
    if (menu_item == SettingsPage::OPTION_SEVEN_SEGMENT_BRIGHTNESS)
        system->outputs.push_back({OutputType::SSD_SET_BRIGHTNESS, {.intValue=(uint8_t)numeric_values[0]}});

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
            system->outputs.push_back({OutputType::SSD_SET_NUMBER_INT, {.intValue=8888}});
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
            edit_mode = NUMERIC_MULTI;
            numeric_min = -500;
            numeric_max = 500;
            numeric_step = 10;
            numeric_values[0] = system->settings.analog_output_range[0][0];
            numeric_values[1] = system->settings.analog_output_range[1][0];
            numeric_values[2] = system->settings.analog_output_range[2][0];
            break;
        case SettingsPage::OPTION_AO_RANGE_HIGH:
            edit_mode = NUMERIC_MULTI;
            numeric_min = -500;
            numeric_max = 500;
            numeric_step = 10;
            numeric_values[0] = system->settings.analog_output_range[0][1];
            numeric_values[1] = system->settings.analog_output_range[1][1];
            numeric_values[2] = system->settings.analog_output_range[2][1];
            break;
    }

    multi_index = edit_mode == NUMERIC_MULTI ? system->inputs.switch_states[0] : 0;
}

void SettingsValuePage::save_value() {
    switch (menu_item) {
        case SettingsPage::OPTION_LIGHT_SOURCE:
            system->settings.light_source = (LightSourceType)(int)numeric_values[0];
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
            for (int i = 0; i < 3; i++) {
                system->settings.analog_output_range[i][0] = numeric_values[i];
            }
            break;
        case SettingsPage::OPTION_AO_RANGE_HIGH:
            for (int i = 0; i < 3; i++) {
                system->settings.analog_output_range[i][1] = numeric_values[i];
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
        if (event.type == DIGITAL_INPUT_RISE && event.long_value == ENCODER_BTN) {
            switch_to_page(PageIndex::SETTINGS);
        }
    }
}


void draw_knob(EPD_Display* display, int center_x, int center_y, double value, const char* title, bool selected, int radius, double angle_range) {
    if (value < 0.0 || value > 1.0) return; // Ensure value is between 0 and 1
    if (angle_range < M_PI || angle_range > 2 * M_PI) return; // Ensure angle range is between 180 and 360 degrees

    // Calculate start and end angles for the knob
    double start_angle = (M_PI + angle_range) / 2.0;
    double end_angle = start_angle - value * angle_range;
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
    int show_val = (int)(value * 100);
    int text_offset;
    if (show_val < 10) text_offset = 6;
    else if (show_val < 100) text_offset = 12;
    else text_offset = 18;
    display->setTextColor(GxEPD_BLACK);
    display->setCursor(center_x - text_offset, center_y + radius * 0.65);
    display->setTextSize(2);
    display->print((int)(value * 100));
    
    // Print title
    int16_t tbx, tby; uint16_t tbw, tbh;
    display->setTextSize(1);
    display->getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
    display->setCursor(center_x - tbw / 2, center_y - radius * 1.05 - tbh);
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
