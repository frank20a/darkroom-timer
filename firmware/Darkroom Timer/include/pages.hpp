#pragma once

#define _USE_MATH_DEFINES

#define FULL_REFRESH_THRESH 100

#include <cmath>
#include <string>
#include <QRCodeGFX.h>

#include "epd.hpp"
#include "inputs.hpp"
#include "system.hpp"
#include "assets.hpp"
#include "string_const.hpp"


void draw_knob(EPD_Display* display, int center_x, int center_y, double value, char title[] = "", bool selected = false, int radius = 30, double angle_range = M_PI * 1.5);
void draw_scrollbar(EPD_Display* display, int val, int max_val);


enum SettingsMenuItem : uint8_t {
    SAVE = 0,
    LIGHT_SOURCE,
    SEVEN_SEGMENT_BRIGHTNESS,
    AO_FUNCTION,
    AO_VOLTAGE_LIMIT,
    AO_VOLTAGE_INPUT,
    AO_COEFFS_1,
    AO_COEFFS_2,
    AO_COEFFS_3,
    AO_COEFFS_4,
    AO_RANGE_LOW,
    AO_RANGE_HIGH,
    ABOUT
};


class EPD_Page {
    public:
        EPD_Page(SystemState* system_state, EPD_Display* display) : system_state(system_state), display(display) {};
        void render();
        void execute_logic() {
            _header_logic();
            logic();
        }

    protected:
        virtual void draw() = 0;
        virtual bool draw_condition() = 0;
        virtual void logic() = 0;
        virtual void on_switch_to() = 0;

        void switch_to_page(PageIndex page) {
            display->set_page(page);
            draw_count = -1; // Force full refresh on next render
        }

        static unsigned int counter;

        EPD_Display* display;
        SystemState* system_state;
        std::string title;
        unsigned int draw_count = -1;
        unsigned long int last_draw_time = 0;
        unsigned int partial_x = 0, partial_y = 0, partial_w = EPD_Type::HEIGHT, partial_h = EPD_Type::WIDTH;

    private:
        bool _always_draw_condition();
        void _draw_header();
        void _header_logic();
};

class TestPage : public EPD_Page {
    public:
        TestPage(SystemState* system_state, EPD_Display* display) : EPD_Page(system_state, display) {
            title = "Test Page";
        };
    private:
        void draw() override;
        bool draw_condition() override;
        void logic() override {};
        void on_switch_to() override {};
};

class SettingsPage : public EPD_Page {
    public:
        SettingsPage(SystemState* system_state, EPD_Display* display) : EPD_Page(system_state, display) {
            title = "Settings";
        };
    private:
        void draw() override;
        bool draw_condition() override;
        void logic() override;
        void on_switch_to() override {
            selected_menu_item = 0;
        };

        int selected_menu_item = 0;
};

class AboutPage : public EPD_Page {
    public:
        AboutPage(SystemState* system_state, EPD_Display* display) : EPD_Page(system_state, display) {
            title = "Settings > About";
        };
    private:
        void draw() override;
        bool draw_condition() override { return false; };
        void logic() override;
        void on_switch_to() override {};
};

