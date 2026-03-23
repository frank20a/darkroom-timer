#include "pages.hpp"
#include "epd.hpp"


void EPD_Page::render(EPD_Display* display) {
    if (millis() - last_draw_time >= draw_interval_ms) {
        last_draw_time = millis();

        if (draw_count >= full_refresh_draws) {
            this->full_render(display);
            draw_count = 0;
        } else {
            this->partial_render(display);
        }
    }
}


void TestPage::full_render(EPD_Display* display) {
    display->setFullWindow();
    display->firstPage();
    do {
        display->fillScreen(GxEPD_WHITE);

        draw_knob(display, 150, 200, 35, (double)(input_data->encoder_data.encoder_value / 100.0));
        draw_knob(display, 250, 200, 35, (double)(input_data->encoder_data.encoder_value / 100.0));
        draw_knob(display, 350, 200, 35, (double)(input_data->encoder_data.encoder_value / 100.0));
    } while (display->nextPage());
}

void TestPage::partial_render(EPD_Display* display) {
    if (input_data->encoder_data.encoder_delta != 0) {
    // if (false) {
        draw_count++;

        display->setPartialWindow(100, 150, 500, 150);
        display->firstPage();
        do {
            draw_knob(display, 150, 200, 35, (double)(input_data->encoder_data.encoder_value / 100.0));
            draw_knob(display, 250, 200, 35, (double)(input_data->encoder_data.encoder_value / 100.0));
            draw_knob(display, 350, 200, 35, (double)(input_data->encoder_data.encoder_value / 100.0));
        } while (display->nextPage());
    }
}


void draw_knob(EPD_Display* display, int center_x, int center_y, int radius, double value, double angle_range) {
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

    display->fillCircle(center_x, center_y, radius * 0.1, GxEPD_BLACK);
    display->drawLine(center_x, center_y, center_x + radius * 0.65 * cos(end_angle), center_y - radius * 0.65 * sin(end_angle), GxEPD_BLACK);
    display->drawLine(center_x, center_y, center_x + radius * 0.65 * cos(end_angle + 0.02), center_y - radius * 0.65 * sin(end_angle + 0.02), GxEPD_BLACK);
    display->drawLine(center_x, center_y, center_x + radius * 0.65 * cos(end_angle - 0.02), center_y - radius * 0.65 * sin(end_angle - 0.02), GxEPD_BLACK);

    int show_val = (int)(value * 100);
    int text_offset;
    if (show_val < 10) text_offset = 6;
    else if (show_val < 100) text_offset = 12;
    else text_offset = 18;

    display->setCursor(center_x - text_offset, center_y + radius * 0.65);
    display->setTextSize(2);
    display->print((int)(value * 100));
        
}