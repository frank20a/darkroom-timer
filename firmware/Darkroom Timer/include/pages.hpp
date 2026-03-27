#pragma once

#define _USE_MATH_DEFINES

#define MIN_DRAW_INTERVAL_MS 100
#define FULL_REFRESH_THRESH 100

#include <cmath>

#include "epd.hpp"
#include "inputs.hpp"
#include "system.hpp"


void draw_knob(EPD_Display* display, int center_x, int center_y, double value, char title[] = "", bool selected = false, int radius = 30, double angle_range = M_PI * 1.5);


class EPD_Page {
public:
    EPD_Page(InputData* input_data, SystemState* system_state) : input_data(input_data), system_state(system_state) {};
    void render(EPD_Display* display);
    void reset_draw_count() { draw_count = -1; };

protected:
    virtual void draw(EPD_Display* display) = 0;
    virtual bool draw_condition() = 0;

    InputData* input_data;
    SystemState* system_state;
    unsigned int draw_count = -1;
    unsigned long int last_draw_time = 0;
    unsigned int partial_x = 0, partial_y = 0, partial_w = EPD_Type::HEIGHT, partial_h = EPD_Type::WIDTH;
};


class TestPage : public EPD_Page {
public:
    TestPage(InputData* input_data, SystemState* system_state) : EPD_Page(input_data, system_state) {};
private:
    void draw(EPD_Display* display) override;
    bool draw_condition() override;
};
