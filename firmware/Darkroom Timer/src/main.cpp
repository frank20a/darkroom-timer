#include "application.hpp"


Application app;


/* ======================== Function definitions =========================== */
void setup() {
    // Initialize Serial
    Serial.begin(115200);
    delay(2000);

    // Initialize SPI
    SPI.setRX(MISO_PIN);
    SPI.setTX(MOSI_PIN);
    SPI.setSCK(SCK_PIN);
    SPI.begin();

    // // Initialize application
    if(!app.begin()) {
        Serial.println("Failed to initialize application.");
        while(1);
    }
}


void loop() {
    app.mainloop_step();
    // Serial.println("Hello, world!");
    // delay(1000);
}
