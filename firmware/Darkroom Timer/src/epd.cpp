#include "epd.hpp"
#include "pages.hpp"


EPD_Display::EPD_Display(int CS, int DC, int RES, int BUSY, SystemState* system) : DisplayType(EPD_Type(CS, DC, RES, BUSY)), system(system) {
    pinMode(CS, OUTPUT);
    digitalWrite(CS, HIGH);
}

bool EPD_Display::init() {
    DisplayType::init();

    setRotation(3);
    setTextColor(GxEPD_BLACK);

    setFullWindow();
    fillScreen(GxEPD_WHITE);

    new TestPage(system, this);
    new TimerPage(system, this);
    new TestStripPage(system, this);
    new LampControlPage(system, this);
    new SettingsPage(system, this);
    new SettingsValuePage(system, this);
    new TablePage(system, this);
    new AboutPage(system, this);

    switch (system->inputs.switch_states[1]) {
        case 0:
            set_page(PageIndex::TIMER);
            break;
        case 1:
            set_page(PageIndex::TEST_STRIP);
            break;
        case 2:
            set_page(PageIndex::LAMP_CONTROL);
            break;
        default:
            set_page(PageIndex::TEST);

    }
    
    return true;
}

void EPD_Display::execute_logic() {
    EPD_Page::pages[current_page]->execute_logic();
}

void EPD_Display::render() {
    EPD_Page::pages[current_page]->render();
}
