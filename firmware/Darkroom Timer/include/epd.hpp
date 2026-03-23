#pragma once

#define ENABLE_GxEPD2_GFX 0

#include <vector>
#include <GxEPD2_BW.h>


typedef GxEPD2_370_GDEY037T03 EPD_Type;
typedef GxEPD2_BW<EPD_Type, EPD_Type::HEIGHT> DisplayType;

class EPD_Page;


class EPD_Display : public DisplayType {
public:
    EPD_Display(int CS, int DC, int RES, int BUSY, InputData* input_data, SystemState* system_state);
    void init();
    void render();

private:
    unsigned short int current_page = 0;
    std::vector<EPD_Page *> pages;
};
