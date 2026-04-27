#pragma once
#include <stdint.h>

struct epd_image_t{
    unsigned char *data;
    uint16_t width;
    uint16_t height;
};

namespace Assets {
    extern const epd_image_t penis;
    extern const epd_image_t calculator;
    extern const epd_image_t floppy;
    extern const epd_image_t light;
    extern const epd_image_t gear;
};
