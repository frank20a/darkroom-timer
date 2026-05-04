#pragma once


#define ENABLE_GxEPD2_GFX 0


#include <map>
#include <GxEPD2_BW.h>

#include "inputs.hpp"
#include "system.hpp"


// typedef for shorter display type name
typedef GxEPD2_370_GDEY037T03 EPD_Type;
typedef GxEPD2_BW<EPD_Type, EPD_Type::HEIGHT> DisplayType;


// Forward declarations to avoid circular dependencies
class EPD_Page;


enum class PageIndex {
    TEST = 0,
    TIMER,
    FSTOP,
    SETTINGS,
    SETTINGS_VALUE,
    ABOUT
};


class EPD_Display : public DisplayType {
    public:
        EPD_Display(int CS, int DC, int RES, int BUSY, SystemState* system);
        bool init();
        void execute_logic();
        void render();
        void set_page(PageIndex page) { current_page = page; }

    private:
        PageIndex current_page = PageIndex::TEST;
        SystemState* system;
};
