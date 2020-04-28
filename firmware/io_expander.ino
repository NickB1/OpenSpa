
uint8_t ioExpanderInit()
{
  io_expander.begin(); //use default address 0

  io_expander.pinMode(epin_o_main, OUTPUT);
  io_expander.pinMode(epin_o_heater, OUTPUT);
  io_expander.pinMode(epin_o_circulation_pump, OUTPUT);
  io_expander.pinMode(epin_o_pump_1, OUTPUT);
  io_expander.pinMode(epin_o_pump_1_speed, OUTPUT);
  io_expander.pinMode(epin_o_pump_2, OUTPUT);
  io_expander.pinMode(epin_o_blower, OUTPUT);
  io_expander.pinMode(epin_o_ozone_generator, OUTPUT);
  io_expander.pinMode(epin_o_light, OUTPUT);

  ioExpanderResetOutputs();

  io_expander.pinMode(epin_i_button_temp_up, INPUT);
  io_expander.pinMode(epin_i_button_temp_down, INPUT);
  io_expander.pinMode(epin_i_button_pump_1, INPUT);
  io_expander.pinMode(epin_i_button_pump_2, INPUT);
  io_expander.pinMode(epin_i_button_blower, INPUT);
  io_expander.pinMode(epin_i_button_light, INPUT);
  io_expander.pinMode(epin_i_pressure_switch, INPUT);

  io_expander.pullUp(epin_i_button_temp_up, HIGH);
  io_expander.pullUp(epin_i_button_temp_down, HIGH);
  io_expander.pullUp(epin_i_button_pump_1, HIGH);
  io_expander.pullUp(epin_i_button_pump_2, HIGH);
  io_expander.pullUp(epin_i_button_blower, HIGH);
  io_expander.pullUp(epin_i_button_light, HIGH);
  io_expander.pullUp(epin_i_pressure_switch, HIGH);

  return 0;
}

void ioExpanderResetOutputs()
{
  io_expander.digitalWrite(epin_o_main, LOW);
  io_expander.digitalWrite(epin_o_heater, LOW);
  io_expander.digitalWrite(epin_o_circulation_pump, LOW);
  io_expander.digitalWrite(epin_o_pump_1, LOW);
  io_expander.digitalWrite(epin_o_pump_1_speed, LOW);
  io_expander.digitalWrite(epin_o_pump_2, LOW);
  io_expander.digitalWrite(epin_o_blower, LOW);
  io_expander.digitalWrite(epin_o_ozone_generator, LOW);
  io_expander.digitalWrite(epin_o_light, LOW);
}

void ioExpanderPinMode(uint8_t pin, uint8_t io_mode)
{
  if (ioReadIOExpanderReset() == 1) //Stop I2C when IO Expander is held in reset
    io_expander.pinMode(pin, io_mode);
}

void ioExpanderWrite(uint8_t pin, uint8_t state)
{
  if (ioReadIOExpanderReset() == 1) //Stop I2C when IO Expander is held in reset
    io_expander.digitalWrite(pin, state);
}

uint8_t ioExpanderRead(uint8_t pin)
{
  if (ioReadIOExpanderReset() == 1) //Stop I2C when IO Expander is held in reset
    return io_expander.digitalRead(pin);
  else
    return 0;
}

void ioExpanderHoldReset(uint8_t state)
{
  if (state)
  {
    digitalWrite(pin_o_io_expander_reset, HIGH);
  }
  else
  {
    digitalWrite(pin_o_io_expander_reset, LOW);
    delay(10);
    ioExpanderInit();
  }
}
