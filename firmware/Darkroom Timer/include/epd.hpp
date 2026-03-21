#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>


typedef GxEPD2_370_GDEY037T03 EPD_Type;
typedef GxEPD2_BW<EPD_Type, EPD_Type::HEIGHT> DisplayType;


class EPD_Display : public DisplayType {
    public:
        EPD_Display(int CS, int DC, int RES, int BUSY) : DisplayType(EPD_Type(CS, DC, RES, BUSY)) {};
        void init();
};


class EPD_Page {

};