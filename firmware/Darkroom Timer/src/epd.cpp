#include "pages.hpp"
#include "epd.hpp"


EPD_Display::EPD_Display(int CS, int DC, int RES, int BUSY, InputData* input_data, SystemState* system_state) : DisplayType(EPD_Type(CS, DC, RES, BUSY)) {
    this->pages.push_back(new TestPage(input_data, system_state));
}

void EPD_Display::init() {
    DisplayType::init();

    this->setRotation(3);
    this->setTextColor(GxEPD_BLACK);

    this->setFullWindow();
    this->fillScreen(GxEPD_WHITE);
}

void EPD_Display::render() {
    pages[current_page]->render(this);
}