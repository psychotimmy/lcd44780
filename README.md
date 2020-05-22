# lcd44780
Display library written in C for I2C HD44780U LCDs with a PCF8574 backpack 

Pre-requisites: A Raspberry Pi running Raspbian Buster with the pigpiod library and daemon installed and running.

This library was written to work with 2x16 and 4x20 LCD displays using a HD44780U controller (or equivalent), 
connected using a PCF8574 backpack to an I2C bus.

It should work with any similar I2C display.

One test program is provided:

lcd44780test - exercise the display (assumes a 4x20 display is being used).

The code is reasonably well documented, if sub-optimal in places.

Tim Holyoake, 22nd May 2020.
