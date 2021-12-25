
OneWire oneWire(pin_io_onewire);

uint8_t oneWireInit()
{

  return 0;
}

void oneWireSearch()
{
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;

  if ( !oneWire.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    oneWire.reset_search();
    delay(250);
    return;
  }

  Serial.print("ROM =");
  for ( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return;
  }
  Serial.println();

  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old oneWire1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a oneWire18x20 family device.");
      return;
  }

  oneWire.reset();
  oneWire.select(addr);
  oneWire.write(0x44, 1);        // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a oneWire.depower() here, but the reset will take care of it.

  present = oneWire.reset();
  oneWire.select(addr);
  oneWire.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = oneWire.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
}

void onewire_poll()
{
  if (openspa_onewire_temp_sensor_1_enable)
    ds18s20_1.poll();

  if (openspa_onewire_temp_sensor_2_enable)
    ds18s20_2.poll();
}



ds18s20::ds18s20(uint8_t * address)
{
  memcpy(m_address, address, 8);
}

void ds18s20::poll()
{
  static unsigned long timestamp;

  switch (m_conversion_state)
  {
    case start_conversion:

      oneWire.reset();
      oneWire.select(m_address);
      oneWire.write(0x44, 1);        // start conversion, with parasite power on at the end
      m_conversion_state = converting;
      timestamp = millis();

      break;

    case converting:

      if ((millis() - timestamp) >= m_conversion_delay_ms)
        m_conversion_state = readout;

      break;

    case readout:
      {
        oneWire.reset();
        oneWire.select(m_address);
        oneWire.write(0xBE);        // Read Scratchpad

        byte data[12] = {0};
        byte type_s = 0;

        for (int i = 0; i < 9; i++)    // we need 9 bytes
        {
          data[i] = oneWire.read();
        }

        // Convert the data to actual temperature
        // because the result is a 16 bit signed integer, it should
        // be stored to an "int16_t" type, which is always 16 bits
        // even when compiled on a 32 bit processor.
        int16_t raw = (data[1] << 8) | data[0];
        if (type_s) {
          raw = raw << 3; // 9 bit resolution default
          if (data[7] == 0x10) {
            // "count remain" gives full 12 bit resolution
            raw = (raw & 0xFFF0) + 12 - data[6];
          }
        } else {
          byte cfg = (data[4] & 0x60);
          // at lower res, the low bits are undefined, so let's zero them
          if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
          else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
          else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
          //// default is 12 bit resolution, 750 ms conversion time
        }
        m_temperature = (float)raw / 16.0;
        m_temperature = round(m_temperature / 0.5) * 0.5;

        m_conversion_state = start_conversion;
      }
      break;

    default:
      m_conversion_state = start_conversion;
      break;
  }
}

float ds18s20::getTemperature()
{
  return m_temperature;
}
