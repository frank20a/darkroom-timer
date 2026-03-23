/* ======================== Library includes ======================== */
#include <Arduino.h>
#include <SPI.h>

#include "inputs.hpp"
#include "system.hpp"
#include "pin_defs.hpp"
#include "epd.hpp"


/* ======================== Variable definitions ======================== */
InputData input_data;
SystemState system_state;
EPD_Display epd(EPD_CS_PIN, EPD_DC_PIN, EPD_RES_PIN, EPD_BUSY_PIN, &input_data, &system_state);


/* ======================== Function declarations ======================== */
void read_inputs();


/* ======================== Function definitions =========================== */
void setup() {
    SPI.setRX(MISO_PIN);
    SPI.setTX(MOSI_PIN);
    SPI.setSCK(SCK_PIN);
    SPI.begin();

    Serial.begin(115200);

    epd.init();
}


void loop() {
    read_inputs();

    epd.render();
}


void read_inputs() {
    static bool flag = true;
    static unsigned long last_change_time = 0;

    if (millis() - last_change_time > 250) {
        input_data.encoder_data.encoder_delta = flag ? 5 : -5;
        input_data.encoder_data.encoder_value += input_data.encoder_data.encoder_delta;

        last_change_time = millis();
    }

    if (input_data.encoder_data.encoder_value >= 100) flag = false;
    if (input_data.encoder_data.encoder_value <= 0) flag = true;
}