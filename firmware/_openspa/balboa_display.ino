
balboa_display::balboa_display(uint8_t pin_o_data, uint8_t pin_i_button_data, uint8_t pin_o_clock) :
  m_pin_o_data(pin_o_data), m_pin_i_button_data(pin_i_button_data), m_pin_o_clock(pin_o_clock)
{
  ioMode(m_pin_o_data, OUTPUT);
  ioMode(m_pin_i_button_data, INPUT);
  ioMode(m_pin_o_clock, OUTPUT);

  ioWrite(m_pin_o_data, LOW);
  ioWrite(m_pin_o_clock, LOW);
}

void balboa_display::ioMode(uint8_t pin, uint8_t io_mode)
{
  pinMode(pin, io_mode);
}

void balboa_display::ioWrite(uint8_t pin, uint8_t state)
{
  digitalWrite(pin, state);
}

uint8_t balboa_display::ioRead(uint8_t pin)
{
  return digitalRead(pin);
}

uint8_t balboa_display::poll()
{
  static uint8_t display_data[balboa_display_data_byte_count] = {0};
  static uint8_t state = 0, bit_cntr = 0, byte_cntr = 0, button_data = 0, button_data_holdoff = 0;
  static unsigned long time_prv = 0;

  switch (state)
  {
    case 0: //reset variables and wait for inter poll delay to pass

      if ((millis() - time_prv) > balboa_display_inter_poll_delay_ms)
      {
        memcpy(display_data, m_display_data, balboa_display_data_byte_count);
        bit_cntr = 0;
        byte_cntr = 0;
        button_data = 0;
        state = 1;
      }

      break;

    case 1: //set data
      if (bit_cntr >= balboa_display_data_bit_count) //Neglect last bit
        ioWrite(m_pin_o_data, LOW);
      else
      {
        ioWrite(m_pin_o_data, (0x01 & (display_data[byte_cntr] >> bit_cntr))); //LSB first
      }
      state = 2;
      break;

    case 2: //set clock
      if (bit_cntr >= balboa_display_data_bit_count) //neglect first bit
        ioWrite(m_pin_o_clock, LOW);
      else
      {
        ioWrite(m_pin_o_clock, HIGH);
      }
      state = 3;
      break;

    case 3: //reset clock
      ioWrite(m_pin_o_clock, LOW);
      if ((bit_cntr == 0) && (byte_cntr == 0) && (ioRead(m_pin_i_button_data) == 0)) //No display detected
      {
        state = 0;
        time_prv = millis();
        return 0;
      }
      else
      {
        //Clock in button data
        if (byte_cntr == (balboa_display_data_byte_count - 1))
        {
          write_byte_bit(&button_data, (0x01 & ioRead(m_pin_i_button_data)), bit_cntr); //Read in bit (LSB first)
        }
        state = 4;
      }

      break;

    case 4: //reset data
      ioWrite(m_pin_o_data, LOW);
      bit_cntr++;
      state = 1;

      if (bit_cntr >= (balboa_display_data_bit_count + 1))
      {
        bit_cntr = 0;
        if (byte_cntr >= (balboa_display_data_byte_count - 1)) //Done
        {
          state = 0;
          time_prv = millis();
          button_data = (0x7F & button_data); //Ommit MSB

          if ((button_data == 0) && (button_data_holdoff == 1)) //Start waiting for data to go back to zero, display sends multiple button commands
          {
            button_data_holdoff = 0;
          }

          if ((button_data != 0) && (m_new_data == 0) && (button_data_holdoff == 0)) //Only write new button data when previous is processed
          {
            m_button_data = button_data;
            m_new_data = 1;
            button_data_holdoff = 1;
            return 1;
          }

        }
        byte_cntr++;
      }
      break;
  }
  return 0;
}

void balboa_display::printDegrees(float temperature, uint8_t arrow)
{
  char display_chars[balboa_display_chars + 1] = {0};

  if (temperature <= 99.9)
  {
    dtostrf(temperature, 4, 1, display_chars);
    //Set characters
    m_display_data[0] = SevenSegmentASCII[display_chars[0] - 0x20];
    m_display_data[1] = SevenSegmentASCII[display_chars[1] - 0x20];
    m_display_data[2] = SevenSegmentASCII[display_chars[3] - 0x20];
    m_display_data[3] = SevenSegmentASCII['C' - 0x20];
    m_display_data[4] = 0x08 | (arrow & 0x01); //set decimal point and optional arrow up
  }
}

void balboa_display::printText(char *text)
{
  m_display_data[0] = SevenSegmentASCII[text[0] - 0x20];
  m_display_data[1] = SevenSegmentASCII[text[1] - 0x20];
  m_display_data[2] = SevenSegmentASCII[text[2] - 0x20];
  m_display_data[3] = SevenSegmentASCII[text[3] - 0x20];
  m_display_data[4] = 0x00;
}

void balboa_display::setLightHeat(uint8_t state)
{
  if (state)
    m_display_data[5] |= balboa_display_light_heat;
  else
    m_display_data[5] &= ~(balboa_display_light_heat);
}

void balboa_display::setLightJets(uint8_t state)
{
  if (state)
    m_display_data[5] |= balboa_display_light_jets;
  else
    m_display_data[5] &= ~(balboa_display_light_jets);
}

void balboa_display::setLightBlower(uint8_t state)
{
  if (state)
    m_display_data[5] |= balboa_display_light_blower;
  else
    m_display_data[5] &= ~(balboa_display_light_blower);
}

uint8_t balboa_display::readButtonData()
{
  m_new_data = 0;
  return m_button_data;
}

void write_byte_bit(uint8_t* write_byte, uint8_t write_bit, uint8_t bit_location)
{
  uint8_t temp;

  if (write_bit)
  {
    *write_byte = *write_byte | (1 << bit_location);
  }
  else
  {
    temp = (1 << bit_location);
    temp = ~temp;
    *write_byte = *write_byte & temp;
  }
}
