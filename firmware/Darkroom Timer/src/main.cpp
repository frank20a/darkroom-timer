#include "application.hpp"


Application app;


/* ======================== Function definitions =========================== */
void setup() {
    // Initialize Serial
    Serial.begin(115200);
    delay(200);

    // Initialize application
    if(!app.begin()) {
        Serial.println("Failed to initialize application.");
        while(1);
    }
}


void loop() {
    app.mainloop_step();
}
