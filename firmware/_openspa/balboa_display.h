#include "ascii_to_7seg.h"

const uint8_t   balboa_display_chars                = 4;
const uint8_t   balboa_display_data_byte_count      = 6;
const uint8_t   balboa_display_data_bit_count       = 7;
const uint16_t  balboa_display_inter_poll_delay_ms  = 50;

const uint8_t   balboa_display_button_temp_up       = 0x10;
const uint8_t   balboa_display_button_temp_down     = 0x50;
const uint8_t   balboa_display_button_lights        = 0x70;
const uint8_t   balboa_display_button_jets          = 0x30;
const uint8_t   balboa_display_button_blower        = 0x60;
const uint8_t   balboa_display_button_hidden        = 0x20;

const uint8_t   balboa_display_char_arrow           = 0x01;
const uint8_t   balboa_display_char_colon           = 0x02;
const uint8_t   balboa_display_char_am              = 0x04;
const uint8_t   balboa_display_char_dec_point       = 0x08;
const uint8_t   balboa_display_char_pm              = 0x10;

const uint8_t   balboa_display_light_heat           = 0x01;
const uint8_t   balboa_display_light_jets           = 0x02;
const uint8_t   balboa_display_light_blower         = 0x08;

class balboa_display
{
  private:
    uint8_t m_pin_o_data = 0, m_pin_i_button_data = 0, m_pin_o_clock = 0;
    uint8_t m_display_data[balboa_display_data_byte_count] = {0};
    uint8_t m_button_data = 0;
    uint8_t m_new_data = 0;
    void ioMode(uint8_t pin, uint8_t io_mode);
    void ioWrite(uint8_t pin, uint8_t state);
    uint8_t ioRead(uint8_t pin);

  public:
    balboa_display(uint8_t pin_o_data, uint8_t pin_i_button_data, uint8_t pin_o_clock);
    uint8_t poll(); //returns 1 when new button data is received
    void printDegrees(float temperature, uint8_t arrow);
    void printText(char *text);
    void setLightHeat(uint8_t state);
    void setLightJets(uint8_t state);
    void setLightBlower(uint8_t state);
    uint8_t readButtonData();
};
