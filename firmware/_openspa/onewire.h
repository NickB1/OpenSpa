
#include <OneWire.h>

OneWire  one_wire(pin_io_onewire);

class ds18s20
{
  public:

    ds18s20(uint8_t * address);
    void poll();
    float getTemperature();

  private:

    static const unsigned long m_conversion_delay_ms = 1000;
    uint8_t m_address[8];
    enum enum_conversion_state { start_conversion, converting, readout };
    enum_conversion_state m_conversion_state = start_conversion;
    float m_temperature;
};

ds18s20 ds18s20_1((uint8_t *) &openspa_onewire_temp_sensor_1_address);
ds18s20 ds18s20_2((uint8_t *) &openspa_onewire_temp_sensor_2_address);
