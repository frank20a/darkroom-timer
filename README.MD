# Darkroom f-stop timer

## Known issues:
- [X] Fix EPD power pin connected to 3.3V instead of 3V3 in KiCad schematic
- [ ] Fix SD card support (or remove it)
- [ ] Fix relays not working on PCB
- [ ] Fix issue with power supply voltage drop
    1. Test with a different power supply
    2. Add voltage measurement feedback to the microcontroller to monitor voltage levels

## Additions for future revisions:
- [ ] Add/Implement the following settings on the menu:
    - [ ] Ramp speed for light output changes
    - [ ] Save & Load templates
- [ ] Move EPD to a separate SPI bus to free up the main SPI bus for other peripherals
- [ ] Move EPD processing to a separate thread to improve responsiveness of the main application
- [ ] Add a safelight switch that bypasses everything
- [ ] Foot pedal support
- [ ] Add sensor support for light and contrast metering
- [ ] Add sensor support for color balancing