
uint8_t ioInit()
{
  pinMode(pin_o_io_expander_reset, OUTPUT);
  digitalWrite(pin_o_io_expander_reset, LOW);

  pinMode(pin_i_io_expander_int, INPUT);
  pinMode(pin_i_io_expander_reset_state, INPUT);

  //Moved to balboa_display class
  //pinMode(pin_i_bb_display_data_in, INPUT);
  //pinMode(pin_o_bb_display_clock, OUTPUT);
  //pinMode(pin_o_bb_display_data_out, OUTPUT);
  //digitalWrite(pin_o_bb_display_clock, LOW);
  //digitalWrite(pin_o_bb_display_data_out, LOW);

  return 0;
}

uint8_t ioReadIOExpanderReset()
{
  return digitalRead(pin_i_io_expander_reset_state);
}
