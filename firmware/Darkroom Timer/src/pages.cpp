#include "pages.hpp"
#include "epd.hpp"


void EPD_Page::render(EPD_Display* display) {
    if (millis() - last_draw_time >= MIN_DRAW_INTERVAL_MS) {
        Serial.println("Rendering page");
        if (draw_count >= FULL_REFRESH_THRESH) {
            
            Serial.println("\tPerforming full refresh");
            display->setFullWindow();
            display->firstPage();
            do {
                draw(display);
            } while (display->nextPage());

            last_draw_time = millis();
            draw_count = 0;
        } else if (draw_condition()) {
            Serial.println("\tPerforming partial refresh");

            draw_count++;

            display->setPartialWindow(partial_x, partial_y, partial_w, partial_h);
            display->firstPage();
            do {
                draw(display);
            } while (display->nextPage());

            last_draw_time = millis();
        }
    }
}


void TestPage::draw(EPD_Display* display) {
    Serial.println("\t\tDrawing TestPage");
    draw_knob(display, 156, 200, (double)(input_data->encoder_data.encoder_value / 100.0), "Cyan");
    draw_knob(display, 260, 200, (double)(input_data->encoder_data.encoder_value / 100.0), "Magenta", true);
    draw_knob(display, 364, 200, (double)(input_data->encoder_data.encoder_value / 100.0), "Yellow");
}

bool TestPage::draw_condition() {
    return input_data->encoder_data.encoder_delta != 0;
}


void draw_knob(EPD_Display* display, int center_x, int center_y, double value, char title[], bool selected, int radius, double angle_range) {
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