#include "epd.hpp"


void EPD_Display::init() {
    DisplayType::init();

    this->setRotation(1);
    this->setTextColor(GxEPD_BLACK);
    this->setFullWindow();
}