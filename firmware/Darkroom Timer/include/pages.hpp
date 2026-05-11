#pragma once

#define _USE_MATH_DEFINES

#define FULL_REFRESH_THRESH 100

#include <cmath>
#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <QRCodeGFX.h>

#include "epd.hpp"
#include "inputs.hpp"
#include "system.hpp"
#include "assets.hpp"
#include "string_const.hpp"


void draw_knob(EPD_Display* display, int center_x, int center_y, int value, const char* title = "", int min_value = 0, int max_value = 100, bool selected = false, bool off = false, int radius = 30, double angle_range = M_PI * 1.5);
void draw_scrollbar(EPD_Display* display, int val, int max_val);
void draw_list(EPD_Display* display, const char** items, int item_count, int selected_index);


class EPD_Page {
    public:
        EPD_Page(SystemState* system, EPD_Display* display, PageIndex index, std::string title) : system(system), display(display), title(title) {
            EPD_Page::pages[index] = this;
        };
        void render();
        void execute_logic();

        static std::map<PageIndex, EPD_Page*> pages;

    protected:
        virtual void draw() = 0;
        virtual void logic() = 0;

        virtual void on_switch_to() {};     // Runs when switching to this page
        virtual void on_switch_from() {};   // Runs when switching away from this page
        virtual String get_title() { return String(title.c_str()); }
        void switch_to_page(PageIndex page) {
            display->set_page(page);
            draw_count = -1; // Force full refresh on next render
            on_switch_from();
        }

        static unsigned int counter;

        EPD_Display* display;
        SystemState* system;
        std::string title;
        bool draw_flag = true;
        unsigned int draw_count = -1;
        unsigned long int last_draw_time = 0;
        unsigned int partial_x = 0, partial_y = 0, partial_w = EPD_Type::HEIGHT, partial_h = EPD_Type::WIDTH;

    private:
        void _draw_header();
        void _header_logic();
        void _menu_selector();

        bool _always_draw_flag = false;
};

class TestPage : public EPD_Page {
    public:
        TestPage(SystemState* system, EPD_Display* display) : EPD_Page(system, display, PageIndex::TEST, "Test Page") {};
    private:
        void draw() override;
        void logic() override;
};

class TimerPage : public EPD_Page {
    public:
        TimerPage(SystemState* system, EPD_Display* display) : EPD_Page(system, display, PageIndex::TIMER, "Timer") {};
    private:
        void draw() override;
        void logic() override;
        void on_switch_to() override;
        void on_switch_from() override;

        float timer_value = 8.0f;
        bool alt_flag = false;
        bool prev_active_flag = false;
};

class TestStripPage : public EPD_Page {
    public:
        TestStripPage(SystemState* system, EPD_Display* display) : EPD_Page(system, display, PageIndex::TEST_STRIP, "Test Strip") {};
    private:
        void draw() override;
        void logic() override;
        void on_switch_to() override;
        void on_switch_from() override;

        float base_value = 8.0f, timer_value;
        bool alt_flag = false;
        bool prev_active_flag = false;
        bool cont_buzzer_flag = false;
        uint8_t test_step = 0;
};

class LampControlPage : public EPD_Page {
    public:
        LampControlPage(SystemState* system, EPD_Display* display) : EPD_Page(system, display, PageIndex::LAMP_CONTROL, "Lamp Control") {};

    private:
        void draw() override;
        void logic() override;
        void on_switch_to() override { system->outputs.preview_state = TESTLIGHT_OFF; }
        void on_switch_from() override { system->outputs.preview_state = TESTLIGHT_OFF; }
};

class SettingsPage : public EPD_Page {
    public:
        SettingsPage(SystemState* system, EPD_Display* display) : EPD_Page(system, display, PageIndex::SETTINGS, "Settings") {
            partial_x = 50;
            partial_y = 35;
            partial_w = EPD_Type::HEIGHT - 55;
            partial_h = 180;
        };
        
        enum SettingsMenuItem : uint8_t {
            OPTION_SAVE = 0,
            OPTION_FSTOP_TABLE,
            OPTION_LIGHT_SOURCE,
            OPTION_SEVEN_SEGMENT_BRIGHTNESS,
            OPTION_BUZZER_FREQUENCY,
            OPTION_AO_FUNCTION,
            OPTION_AO_VOLTAGE_LIMIT,
            OPTION_AO_VOLTAGE_INPUT,
            OPTION_AO_COEFFS_1,
            OPTION_AO_COEFFS_2,
            OPTION_AO_COEFFS_3,
            OPTION_AO_COEFFS_4,
            OPTION_AO_RANGE_LOW,
            OPTION_AO_RANGE_HIGH,
            OPTION_ABOUT,
            OPTION_RESET_SETTINGS
        };
    private:
        void draw() override;
        void logic() override;
        void on_switch_to() override { selected_menu_item = 0; };

        int selected_menu_item = 0;
};

class SettingsValuePage : public EPD_Page {
    public:
        SettingsValuePage(SystemState* system, EPD_Display* display) : EPD_Page(system, display, PageIndex::SETTINGS_VALUE, "Settings > ") {};
        void set_parameters(SettingsPage::SettingsMenuItem menu_item) { this->menu_item = menu_item; }

    private:
        void draw() override;
        void logic() override;
        void on_switch_to() override;
        void save_value();
        String get_title() override { 
            return EPD_Page::get_title() + String(string_const::SettingsMenuItems[menu_item]);
        }

        SettingsPage::SettingsMenuItem menu_item;
        
        enum ValueEditMode {
            MULTI_SELECTION,
            NUMERIC_SINGLE,
            NUMERIC_MULTI,
        } edit_mode;
        double numeric_min, numeric_max, numeric_step, numeric_multiplier;
        double numeric_values[3] = {0};
        const char** multi_options = nullptr;
        const char* multi_labels[3] = {
            "Cyan",
            "Magenta",
            "Yellow"
        };
        const char* unit = "";
        unsigned int decimal_points = 0;
        uint8_t multi_index;
};

class AboutPage : public EPD_Page {
    public:
        AboutPage(SystemState* system, EPD_Display* display) : EPD_Page(system, display, PageIndex::ABOUT, "Settings > About") { };

    private:
        void draw() override;
        void logic() override;
};

class TablePage : public EPD_Page {
    public:
        TablePage(SystemState* system, EPD_Display* display) : EPD_Page(system, display, PageIndex::TABLE, "F-Stop Table") { };

    private:
        void draw() override;
        void logic() override;
        String get_title() override { return String(title.c_str()) + (diff ? " (Diff)" : " (Value)"); }

        float base_value = 8.0f;
        bool diff = false;
};
