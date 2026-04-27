#include "epd.hpp"
#include "pages.hpp"


EPD_Display::EPD_Display(int CS, int DC, int RES, int BUSY, SystemState* system_state) : DisplayType(EPD_Type(CS, DC, RES, BUSY)), system_state(system_state) {
    pinMode(CS, OUTPUT);
    digitalWrite(CS, HIGH);

    pages[PageIndex::TEST] = new TestPage(system_state, this);
    pages[PageIndex::SETTINGS] = new SettingsPage(system_state, this);
    pages[PageIndex::ABOUT] = new AboutPage(system_state, this);
}

bool EPD_Display::init() {
    DisplayType::init();

    setRotation(3);
    setTextColor(GxEPD_BLACK);

    setFullWindow();
    fillScreen(GxEPD_WHITE);
    
    return true;
}

void EPD_Display::execute_logic() {
    pages[current_page]->execute_logic();
}

void EPD_Display::render() {
    pages[current_page]->render();
}
