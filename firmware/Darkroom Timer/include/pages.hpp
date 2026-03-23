#pragma once

#define _USE_MATH_DEFINES

#include <cmath>

#include "inputs.hpp"
#include "system.hpp"


class EPD_Display;

void draw_knob(EPD_Display* display, int center_x, int center_y, int radius, double value, double angle_range = M_PI * 1.5);


class EPD_Page {
public:
    EPD_Page(InputData* input_data, SystemState* system_state) : input_data(input_data), system_state(system_state) {};
    void render(EPD_Display* display);
    virtual void full_render(EPD_Display* display) = 0;
    virtual void partial_render(EPD_Display* display) = 0;
    void reset_draw_count() { draw_count = -1; };

    static const unsigned int draw_interval_ms = 100;
    static const unsigned int full_refresh_draws = 50;
protected:
    InputData* input_data;
    SystemState* system_state;
    unsigned int draw_count = -1;
    unsigned long int last_draw_time = 0;
};


class TestPage : public EPD_Page {
public:
    TestPage(InputData* input_data, SystemState* system_state) : EPD_Page(input_data, system_state) {};
    void full_render(EPD_Display* display) override;
    void partial_render(EPD_Display* display) override;
private:

};
