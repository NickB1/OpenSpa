
#include "Adafruit_MCP23017.h"

Adafruit_MCP23017 io_expander;

const uint8_t epin_o_main                 = 0;
const uint8_t epin_o_heater               = 1;
const uint8_t epin_o_circulation_pump     = 2;
const uint8_t epin_o_pump_1               = 3;
const uint8_t epin_o_pump_1_speed         = 4;
const uint8_t epin_o_pump_2               = 5;
const uint8_t epin_o_blower               = 6;
const uint8_t epin_o_ozone_generator      = 7;

const uint8_t epin_o_light                = 8;
const uint8_t epin_i_button_temp_up       = 9;
const uint8_t epin_i_button_temp_down     = 10;
const uint8_t epin_i_button_pump_1        = 11;
const uint8_t epin_i_button_pump_2        = 12;
const uint8_t epin_i_button_blower        = 13;
const uint8_t epin_i_button_light         = 14;
const uint8_t epin_i_pressure_switch      = 15;
