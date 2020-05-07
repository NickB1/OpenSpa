
const uint8_t pin_o_io_expander_reset               = 16;
const uint8_t pin_i_io_expander_int                 = 14;
const uint8_t pin_i_io_expander_reset_state         = 12; //pin_o_io_expander_reset OR'ed with external heater overtemp detection
const uint8_t pin_i_bb_display_data_in              = 13;
const uint8_t pin_o_bb_display_clock                = 15;
const uint8_t pin_o_bb_display_data_out             = 2;
const uint8_t pin_io_onewire                        = 0;
const uint8_t pin_io_i2c_sda                        = 4;
const uint8_t pin_o_i2c_scl                         = 5;

uint8_t pin_o_io_expander_reset_state   = 0;
