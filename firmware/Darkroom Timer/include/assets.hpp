#pragma once
#include <stdint.h>

typedef struct {
    const uint8_t* data;
    uint16_t width;
    uint16_t height;
} epd_image_t;

extern const epd_image_t penis;