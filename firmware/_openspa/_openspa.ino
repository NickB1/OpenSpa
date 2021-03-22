#include "_openspa.h"
#include "console.h"
#include "io.h"
#include "i2c.h"
#include "onewire.h"
#include "io_expander.h"
#include "hot_tub.h"
#include "balboa_display.h"
#include "wifi.h"

#define TIMER_INTERVAL_MS        10

//ESP8266Timer ITimer;

hot_tub jacuzzi(epin_o_main,    epin_o_heater,          epin_o_circulation_pump,
                epin_o_pump_1,  epin_o_pump_1_speed,    epin_o_pump_2,
                epin_o_blower,  epin_o_ozone_generator, epin_o_light,
                epin_i_pressure_switch);

balboa_display bb_display(pin_o_bb_display_data_out, pin_i_bb_display_data_in, pin_o_bb_display_clock);

uint8_t openspa_error = 0;

void setup()
{
  consoleInit();
  ioInit();
  i2cInit();
  ioExpanderInit();
  jacuzziInit();
  if (openspa_wifi_enable)
    wifiInit();
  delay(10);
  consolePrintCommands();
  
  /*pinMode(pin_io_onewire, OUTPUT); 
  analogWrite(pin_io_onewire, 512); //Working*/
}

void loop()
{
  if (openspa_wifi_enable)
    wifiHandler();

  openspaErrorHandler();

  openspa_error = jacuzzi.poll();

  displayHandler();
  consoleHandler();
}

uint8_t jacuzziInit()
{
  jacuzzi.setMaxTemperature(openspa_max_temperature);
  jacuzzi.setMinTemperature(openspa_min_temperature);
  jacuzzi.setDesiredTemperature(openspa_init_desired_temp);

  jacuzzi.setMaxTotalPower(openspa_power_total_max);
  jacuzzi.setHeaterPower(openspa_power_heater);
  jacuzzi.setCircPumpPower(openspa_power_pump_circ);
  jacuzzi.setPump_1_Power(openspa_power_pump_1);
  jacuzzi.setPump_2_Power(openspa_power_pump_2);
  jacuzzi.setBlowerPower(openspa_power_blower);

  jacuzzi.setPump_1_Timing(openspa_runtime_pump_1, openspa_resttime_pump_1);
  jacuzzi.setPump_2_Timing(openspa_runtime_pump_2, openspa_resttime_pump_2);
  jacuzzi.setBlowerTiming(openspa_runtime_blower, openspa_resttime_blower);

  jacuzzi.setFilteringSettings(openspa_filter_window_start_time, openspa_filter_window_stop_time, openspa_ozone_window_start_time,
                       openspa_ozone_window_stop_time, openspa_filter_daily_cycles, openspa_filter_time);
  jacuzzi.setHeatingSettings(openspa_heating_timeout, openspa_heating_timeout_delta_degrees);
  jacuzzi.setFlushingSettings(openspa_flush_window_start_time, openspa_flush_window_stop_time, openspa_flush_daily_cycles,
                      openspa_flush_time_pump_1, openspa_flush_time_pump_2, openspa_flush_time_blower);

  return 0;
}

uint8_t openspaErrorHandler()
{
  static uint8_t io_expander_reset_state_prv = true;

  if ((io_expander_reset_state_prv == false) and (ioReadIOExpanderReset() == true)) //Reinit when reset is released (error reset button on pcb)
  {
    delay(10);
    io_expander_reset_state_prv = false;
    openspaReset();
  }

  io_expander_reset_state_prv = ioReadIOExpanderReset();

  if (io_expander_reset_state_prv == false)
  {
    openspa_error = 255; //External error (Heater overtemp)
  }

  return 0;
}

void openspaReset()
{
  ioExpanderInit();
  digitalWrite(pin_o_io_expander_reset, HIGH);
  jacuzzi.reset();
  openspa_error = 0;
}
