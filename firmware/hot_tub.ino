
hot_tub::hot_tub(uint8_t pin_o_main,   uint8_t pin_o_heater,           uint8_t pin_o_circulation_pump,
                 uint8_t pin_o_pump_1, uint8_t pin_o_pump_1_speed,     uint8_t pin_o_pump_2,
                 uint8_t pin_o_blower, uint8_t pin_o_ozone_generator,  uint8_t pin_o_light,
                 uint8_t pin_i_pressure_switch)
{
  m_periph_main.pin = pin_o_main;
  m_periph_heater.pin = pin_o_heater;
  m_periph_circ_pump.pin = pin_o_circulation_pump;
  m_periph_pump_1.pin = pin_o_pump_1;
  m_periph_pump_1.pin_speed = pin_o_pump_1_speed;
  m_periph_pump_2.pin = pin_o_pump_2;
  m_periph_blower.pin = pin_o_blower;
  m_periph_ozone.pin = pin_o_ozone_generator;
  m_periph_light.pin = pin_o_light;

  m_pin_pressure_switch = pin_i_pressure_switch;

  this->reset();
}


//External callbacks

void hot_tub::ioWrite(uint8_t pin, uint8_t state)
{
  ioExpanderWrite(pin, state);
}

uint8_t hot_tub::ioRead(uint8_t pin)
{
  return ioExpanderRead(pin);
}

void hot_tub::readTemp()
{
  static uint16_t timestamp = 0;
  if ((this->timePassed(timestamp)) > 1) //Sample every second
  {
    m_current_temperature = thermistorRead();
    timestamp = this->timeStamp();
  }
}

unsigned long hot_tub::timeStamp()
{
  return (millis() / 1000);
}

unsigned long hot_tub::timePassed(unsigned long timestamp)
{
  return ((millis() / 1000) - timestamp);
}

uint16_t hot_tub::getTimeOfDay()
{
  return timeOfDay();
}

uint16_t hot_tub::getTimeDiffDecimalHours(uint16_t time_start, uint16_t time_stop)
{
  uint16_t time_start_hours = 0, time_stop_hours = 0;
  uint16_t time_start_decimal = 0, time_stop_decimal = 0;
  time_start_hours = time_start / 100;
  time_stop_hours = time_stop / 100;

  time_start_decimal = ((time_start - (time_start_hours * 100)) / 0.60) + (time_start_hours * 100);
  time_stop_decimal = ((time_stop - (time_stop_hours * 100)) / 0.60) + (time_stop_hours * 100);

  return (time_stop_decimal - time_start_decimal);
}




void hot_tub::reset()
{
  this->setHeater(false);
  this->setCircPump(false);
  this->setPump_1(false, false);
  this->setPump_2(false, false);
  this->setBlower(false, false);
  this->setLight(false, false);
  this->setOzone(false);

  m_status = 0;
  m_error_code = 0;

  filter_heater_state_machine(true); //Reset state machine
}

void hot_tub::allOff()
{
  this->setHeater(false);
  this->setCircPump(false);
  this->setPump_1(false, false);
  this->setPump_2(false, false);
  this->setBlower(false, false);
  this->setLight(false, false);
  this->setOzone(false);
}



uint8_t hot_tub::poll()
{
  uint8_t out_of_use = 0;
  this->errorChecking();

  this->readTemp();
  this->checkRuntime();
  
  if (m_error_code == 0)
  {
    out_of_use = (this->outOfUse());
    this->filtering(out_of_use, out_of_use); //Force filter cycle whitout ozone when out of use
    this->heating();
    this->filter_heater_state_machine(false);
    this->flushing();
  }
  else
    this->allOff();

  return m_error_code;
}



uint8_t hot_tub::outOfUse()
{
  const uint16_t min_usage_time_s = 300;
  static uint16_t timestamp = 0;
  static uint8_t state_prv = 0;
  uint8_t out_of_use = 0;

  if ((getPumpsAndBlowerState() != 0) and (state_prv == 0)) //In use
    timestamp = this->timeStamp();
  else if ((getPumpsAndBlowerState() == 0) and (state_prv != 0)) //Out of use
  {
    if ((this->timePassed(timestamp) > min_usage_time_s))
    {
      out_of_use = 1;
    }
  }

  state_prv = getPumpsAndBlowerState();
  return out_of_use;
}



uint8_t hot_tub::errorChecking()
{
  if (m_current_temperature > m_max_temperature)
  {
    m_error_code = hot_tub_error_overtemp;
  }

  if ((m_current_temperature < 0) & (hot_tub_debug == 0))
  {
    m_error_code = hot_tub_error_temp_sensor;
  }


  if (this->getHeaterState())
  {
    if (this->getCircPumpState() == false)
    {
      m_error_code = hot_tub_error_heating_on_without_circ_pump;
    }

    if ((this->timePassed(m_periph_heater.timestamp)) > m_heating_timeout)
    {
      if (m_current_temperature < (m_initial_temperature + m_heating_timeout_delta_degrees))
      {
        m_error_code = hot_tub_error_heating_timeout;
      }
    }
  }

  if ((this->getPressureSwitchState() == true) and (this->getCircPumpState() == false) and ((this->timePassed(m_periph_circ_pump.timestamp)) > m_pressure_switch_max_delay_s))
  {
    m_error_code = hot_tub_error_pressure_switch_always_on;
  }


  static uint8_t circ_pump_state_prv = 0;

  if ((this->getCircPumpState() == true) and (circ_pump_state_prv = false))
  {
    if ((this->timePassed(m_periph_circ_pump.timestamp)) > m_pressure_switch_max_delay_s)
    {
      if (this->getPressureSwitchState() == false)
      {
        m_error_code = hot_tub_error_pressure_switch_failed;
      }
      circ_pump_state_prv = this->getCircPumpState();
    }
  }
  else
  {
    circ_pump_state_prv = this->getCircPumpState();
  }

  return m_error_code;
}




void hot_tub::setFiltering(uint16_t filter_window_start_time, uint16_t filter_window_stop_time, uint16_t ozone_window_start_time,
                           uint16_t ozone_window_stop_time, uint16_t filter_daily_cycles, uint16_t filter_time_s)
{
  m_filter_window_start_time = filter_window_start_time;
  m_filter_window_stop_time = filter_window_stop_time;
  m_ozone_window_start_time = ozone_window_start_time;
  m_ozone_window_stop_time = ozone_window_stop_time;
  m_filter_daily_cycles = filter_daily_cycles;
  m_filter_time_s = filter_time_s;
}

void hot_tub::setHeating(uint16_t heating_timeout, float heating_timeout_delta_degrees)
{
  m_heating_timeout = heating_timeout;
  m_heating_timeout_delta_degrees = heating_timeout_delta_degrees;
}

void hot_tub::setFlushing(uint16_t flush_window_start_time, uint16_t flush_window_stop_time, uint16_t flush_daily_cycles,
                          uint16_t flush_time_pump_1_s, uint16_t flush_time_pump_2_s, uint16_t flush_time_blower_s)
{
  m_flush_window_start_time = flush_window_start_time;
  m_flush_window_stop_time = flush_window_stop_time;
  m_flush_daily_cycles = flush_daily_cycles;
  m_flush_time_pump_1_s = flush_time_pump_1_s;
  m_flush_time_pump_2_s = flush_time_pump_2_s;
  m_flush_time_blower_s = flush_time_blower_s;
}

uint16_t hot_tub::getCyclePeriod(uint16_t cycle_window_start_time, uint16_t cycle_window_stop_time, uint16_t cycle_time_s, uint16_t cycles)
{
  uint16_t cycle_period = 0, hours = 0;

  cycle_period = ((getTimeDiffDecimalHours(cycle_window_start_time, cycle_window_stop_time) - (cycle_time_s / 36)) / cycles);
  hours = cycle_period / 100;
  return (((cycle_period - (hours * 100)) * 0.60) + (hours * 100));
}

uint16_t hot_tub::getNextCycleTime(uint16_t cycle_period)
{
  uint16_t next_cycle = 0, hours = 0;

  next_cycle = (this->getTimeOfDay() + cycle_period);
  hours = next_cycle / 100;
  next_cycle -= (hours * 100);
  if (next_cycle >= 60)
  {
    hours++;
    next_cycle -= 60;
  }
  next_cycle += (hours * 100);
  return next_cycle;
}

void hot_tub::filtering(uint8_t force_filter_cycle, uint8_t force_no_ozone)
{
  static uint16_t time_of_day = 0;
  static unsigned long timestamp = 0;
  enum enum_filtering_state { idle, start_filtering, filtering };
  static enum_filtering_state filtering_state = start_filtering; //start filtering on startup
  static uint16_t cycle_period = this->getCyclePeriod(m_filter_window_start_time, m_filter_window_stop_time, m_filter_time_s, m_filter_daily_cycles);
  static uint8_t filter_cycle = 0;

  time_of_day = this->getTimeOfDay();

  switch (filtering_state)
  {
    case idle:
      if (force_filter_cycle)
        filtering_state = start_filtering;

      if ((m_filter_window_start_time < time_of_day) and (m_filter_window_stop_time > time_of_day))
      {
        if (time_of_day >= m_filter_next_cycle_time)
          filtering_state = start_filtering;
      }
      else
      {
        m_filter_next_cycle_time = m_filter_window_start_time;
      }

      if ((m_ozone_window_start_time < time_of_day) and (m_ozone_window_stop_time > time_of_day) and (force_no_ozone == 0)) //Check if ozone may be enabled
        m_filtering_ozone_enabled = true;
      else
        m_filtering_ozone_enabled = false;
      break;

    case start_filtering:
      m_filter_next_cycle_time = this->getNextCycleTime(cycle_period);
      timestamp = this->timeStamp();
      filter_cycle++;
      m_filtering_run = true;
      filtering_state = filtering;
      break;

    case filtering:
      if ((this->timePassed(timestamp)) >= m_filter_time_s) //check if done
      {
        m_filtering_run = false;
        filtering_state = idle;
      }
      break;

    default:
      filtering_state = start_filtering;
      break;
  }
}

void hot_tub::heating()
{
  if ((m_current_temperature - m_desired_temperature) <=  m_desired_temperature_delta_minus)
  {
    m_heating_run = true;
  }
  else if ((m_current_temperature - m_desired_temperature) >=  m_desired_temperature_delta_plus)
  {
    m_heating_run = false;
  }
}

void hot_tub::filter_heater_state_machine(uint8_t reset)
{
  enum enum_filter_heater_state { idle, start_pump, filtering, start_heater, heating, pause, unpause};
  static enum_filter_heater_state filter_heater_state = pause;
  static unsigned long timestamp = 0;
  uint8_t unpause_delay = 10;

  if (reset)
  {
    timestamp = 0;
    filter_heater_state = pause;
  }

  switch (filter_heater_state)
  {
    case idle:
      m_status = hot_tub_status_idle;
      if (m_filtering_run | m_heating_run)
      {
        if ((this->getPumpsAndBlowerState()) == 0) //only start when all pumps and blower are turned off
          filter_heater_state = start_pump;
      }
      break;

    case start_pump:
      timestamp = this->timeStamp();
      this->setCircPump(true);

      if (this->getPumpsAndBlowerState()) //Pause when a pump or blower is enabled
        filter_heater_state = pause;

      if (m_filtering_ozone_enabled)
        this->setOzone(true);

      if (m_heating_run == true)
        filter_heater_state = start_heater;
      else if (m_filtering_run == true)
        filter_heater_state = filtering;

      break;

    case filtering:
      m_status = hot_tub_status_filtering;

      if (this->getPumpsAndBlowerState()) //Pause when a pump or blower is enabled
        filter_heater_state = pause;

      if (m_filtering_run == false)
      {
        filter_heater_state = idle;
        this->setCircPump(false);
        this->setOzone(false);
      }

      else if (m_heating_run == true)
        filter_heater_state = start_heater;

      break;

    case start_heater:
      if (this->getPumpsAndBlowerState()) //Pause when a pump or blower is enabled
        filter_heater_state = pause;

      if ((this->timePassed(timestamp)) >= m_heater_turn_on_delay_s)
      {
        this->setHeater(true);
        filter_heater_state = heating;
      }

      break;

    case heating:
      if (m_filtering_run)
        m_status = hot_tub_status_filtering_heating;
      else
        m_status = hot_tub_status_heating;

      if (this->getPumpsAndBlowerState()) //Pause when a pump or blower is enabled
        filter_heater_state = pause;

      if (m_heating_run == false)
      {
        this->setHeater(false);
        filter_heater_state = filtering;
      }

      break;

    case pause:
      this->setCircPump(false);
      this->setHeater(false);
      this->setOzone(false);

      if (this->getPumpsAndBlowerState() == 0) //only start back up when all pumps and blower are turned off
      {
        timestamp = this->timeStamp();
        filter_heater_state = unpause;
      }
      break;

    case unpause:
      if (this->getPumpsAndBlowerState()) //Pause when a pump or blower is enabled
        filter_heater_state = pause;

      if ((this->timePassed(timestamp)) > unpause_delay)
        filter_heater_state = idle;
      break;

    default:
      this->setCircPump(false);
      this->setHeater(false);
      this->setOzone(false);
      filter_heater_state = idle;
      break;
  }
}

void hot_tub::flushing()
{
  uint16_t time_of_day = 0;
  static unsigned long timestamp = 0;
  enum enum_flushing_state { idle, start_flushing, start_blower, flushing_blower, start_pump_1, flushing_pump_1, start_pump_2,  flushing_pump_2 };
  static enum_flushing_state flushing_state = start_flushing; //start flushing on startup
  static uint16_t cycle_period = this->getCyclePeriod(m_flush_window_start_time, m_flush_window_stop_time, m_flush_time_pump_1_s, m_flush_daily_cycles);
  static uint8_t flush_cycle = 0;
  uint8_t flush_inter_delay_s = 3;
  static uint8_t pumps_and_blower_state_prv = 0, flushing_started = 0;

  time_of_day = this->getTimeOfDay();

  if ((m_flush_window_start_time < time_of_day) and (m_flush_window_stop_time > time_of_day))
  {
    flushing_started = 1;

    switch (flushing_state)
    {
      case idle:
        if (getPumpsAndBlowerState()) //Hold off flushing when pumps or blower are in use
        {
          pumps_and_blower_state_prv = 1;
        }
        else if (pumps_and_blower_state_prv == 1)
        {
          pumps_and_blower_state_prv = 0;
          m_flush_next_cycle_time = this->getNextCycleTime(cycle_period);
        }
        else
        {
          if (time_of_day >= m_flush_next_cycle_time)
            flushing_state = start_flushing;
        }
        break;

      case start_flushing:
        m_flush_next_cycle_time = this->getNextCycleTime(cycle_period);
        timestamp = this->timeStamp();
        flush_cycle++;
        flushing_state = start_blower;
        break;

      case start_blower:
        if ((this->timePassed(timestamp)) >= flush_inter_delay_s)
        {
          timestamp = this->timeStamp();
          setBlower(true, false);
          flushing_state = flushing_blower;
        }
        break;

      case flushing_blower:
        if ((this->timePassed(timestamp)) >= m_flush_time_blower_s)
        {
          timestamp = this->timeStamp();
          setBlower(false, false);
          flushing_state = start_pump_1;
        }
        break;

      case start_pump_1:
        if ((this->timePassed(timestamp)) >= flush_inter_delay_s)
        {
          timestamp = this->timeStamp();
          setPump_1(true, false);
          flushing_state = flushing_pump_1;
        }
        break;

      case flushing_pump_1:
        if ((this->timePassed(timestamp)) >= m_flush_time_pump_1_s)
        {
          timestamp = this->timeStamp();
          setPump_1(false, false);
          flushing_state = start_pump_2;
        }
        break;

      case start_pump_2:
        if ((this->timePassed(timestamp)) >= flush_inter_delay_s)
        {
          timestamp = this->timeStamp();
          setPump_2(true, false);
          flushing_state = flushing_pump_2;
        }
        break;

      case flushing_pump_2:
        if ((this->timePassed(timestamp)) >= m_flush_time_pump_2_s)
        {
          timestamp = this->timeStamp();
          setPump_2(false, false);
          flushing_state = idle;
        }
        break;

      default:
        flushing_state = idle;
        break;
    }
  }
  else
  {
    if (flushing_started) //Make sure everything is turned of when flushing is interrupted
    {
      setBlower(false, false);
      setPump_1(false, false);
      setPump_2(false, false);
      flushing_started = 0;
    }

    m_flush_next_cycle_time = m_flush_window_start_time;
    flushing_state = start_flushing;
    flush_cycle = 0;
  }
}



void hot_tub::checkRuntime()
{
  if (this->getPump_1_State() and ((this->timePassed(m_periph_pump_1.timestamp)) >= m_periph_pump_1.runtime))
    this->setPump_1(false, false);

  if (this->getPump_2_State() and ((this->timePassed(m_periph_pump_2.timestamp)) >= m_periph_pump_2.runtime))
    this->setPump_2(false, false);

  if (this->getBlowerState() and ((this->timePassed(m_periph_blower.timestamp)) >= m_periph_blower.runtime))
    this->setBlower(false, false);
}




uint8_t hot_tub::getErrorCode()
{
  return m_error_code;
}

uint8_t hot_tub::getStatus()
{
  return m_status;
}

uint8_t hot_tub::getFilteringStatus()
{
  return m_filtering_run;
}

uint16_t hot_tub::getFilteringNextCycleTime()
{
  return m_filter_next_cycle_time;
}

uint8_t hot_tub::getHeatingStatus()
{
  return m_heating_run;
}

uint16_t hot_tub::getFlushingNextCycleTime()
{
  return m_flush_next_cycle_time;
}




void hot_tub::setMinTemperature(float min_temperature)
{
  m_min_temperature = min_temperature;
}

void hot_tub::setMaxTemperature(float max_temperature)
{
  m_max_temperature = max_temperature;
}

void hot_tub::setDesiredTemperature(float desired_temperature)
{
  if ((desired_temperature <= m_max_temperature) & (desired_temperature >= m_min_temperature))
    m_desired_temperature = desired_temperature;
}

void hot_tub::increaseDesiredTemperature()
{
  m_desired_temperature += m_desired_temperature_increment;
  if (m_desired_temperature > m_max_temperature)
    m_desired_temperature = m_max_temperature;
}

void hot_tub::decreaseDesiredTemperature()
{
  m_desired_temperature -= m_desired_temperature_increment;
  if (m_desired_temperature < m_min_temperature)
    m_desired_temperature = m_min_temperature;
}

float hot_tub::currentTemperature()
{
  this->readTemp();
  return m_current_temperature;
}

float hot_tub::desiredTemperature()
{
  return m_desired_temperature;
}




void hot_tub::setMaxTotalPower(uint16_t power)
{
  m_max_total_power = power;
}




void hot_tub::setHeaterPower(uint16_t power)
{
  m_periph_heater.power = power;
}

void hot_tub::setHeater(uint8_t state)
{
  if (m_periph_heater.state != state)
  {
    m_periph_heater.state = state;
    this->ioWrite(m_periph_heater.pin, m_periph_heater.state);
  }
  m_initial_temperature = m_current_temperature;
  m_periph_heater.timestamp = this->timeStamp();
}

uint8_t hot_tub::getHeaterState()
{
  return m_periph_heater.state;
}




void hot_tub::setCircPumpPower(uint16_t power)
{
  m_periph_circ_pump.power = power;
}

void hot_tub::setCircPump(uint8_t state)
{
  if (m_periph_circ_pump.state != state)
  {
    m_periph_circ_pump.state = state;
    this->ioWrite(m_periph_circ_pump.pin, m_periph_circ_pump.state);
  }
  m_periph_circ_pump.timestamp = this->timeStamp();
}

uint8_t hot_tub::getCircPumpState()
{
  return m_periph_circ_pump.state;
}


uint8_t hot_tub::getPressureSwitchState()
{
  if (hot_tub_debug)
    return (this -> getCircPumpState());
  else
    return (!(this->ioRead(m_pin_pressure_switch))); //Active low
}




void hot_tub::setPump_1_Power(uint16_t power)
{
  m_periph_pump_1.power = power;
}

void hot_tub::setPump_1_Timing(uint16_t runtime, uint16_t resttime)
{
  m_periph_pump_1.runtime = runtime;
  m_periph_pump_1.resttime = resttime;
}

void hot_tub::setPump_1(uint8_t state, uint8_t toggle)
{
  static uint8_t state_prv = 0;
  state_prv = m_periph_pump_1.state;

  if (toggle)
  {
    m_periph_pump_1.state += 1;
    if (m_periph_pump_1.state >= 3)
      m_periph_pump_1.state = 0;
  }
  else
  {
    if (m_periph_pump_1.state > 2)
      m_periph_pump_1.state = 2;

    m_periph_pump_1.state = state;
  }

  if (state_prv != m_periph_pump_1.state)
  {

    this->ioWrite(m_periph_pump_1.pin, (((m_periph_pump_1.state) | (m_periph_pump_1.state >> 1)) & 0x01));
    this->ioWrite(m_periph_pump_1.pin_speed, ((m_periph_pump_1.state >> 1) & 0x01));
  }

  m_periph_pump_1.timestamp = this->timeStamp();
}

uint8_t hot_tub::getPump_1_State()
{
  return m_periph_pump_1.state;
}




void hot_tub::setPump_2_Power(uint16_t power)
{
  m_periph_pump_2.power = power;
}

void hot_tub::setPump_2_Timing(uint16_t runtime, uint16_t resttime)
{
  m_periph_pump_2.runtime = runtime;
  m_periph_pump_2.resttime = resttime;
}

void hot_tub::setPump_2(uint8_t state, uint8_t toggle)
{
  static uint8_t state_prv = 0;
  state_prv = m_periph_pump_2.state;

  if (toggle)
    m_periph_pump_2.state = !m_periph_pump_2.state;
  else
    m_periph_pump_2.state = state;

  if (state_prv != m_periph_pump_2.state)
    this->ioWrite(m_periph_pump_2.pin, m_periph_pump_2.state);

  m_periph_pump_2.timestamp = this->timeStamp();
}

uint8_t hot_tub::getPump_2_State()
{
  return m_periph_pump_2.state;
}




void hot_tub::setBlowerPower(uint16_t power)
{
  m_periph_blower.power = power;
}

void hot_tub::setBlowerTiming(uint16_t runtime, uint16_t resttime)
{
  m_periph_blower.runtime = runtime;
  m_periph_blower.resttime = resttime;
}

void hot_tub::setBlower(uint8_t state, uint8_t toggle)
{
  static uint8_t state_prv = 0;
  state_prv = m_periph_blower.state;

  if (toggle)
    m_periph_blower.state = !m_periph_blower.state;
  else
    m_periph_blower.state = state;

  if (state_prv != m_periph_blower.state)
    this->ioWrite(m_periph_blower.pin, m_periph_blower.state);

  m_periph_blower.timestamp = this->timeStamp();
}

uint8_t hot_tub::getBlowerState()
{
  return m_periph_blower.state;
}

uint8_t hot_tub::getPumpsAndBlowerState()
{
  return ((this->getPump_1_State()) or (this->getPump_2_State()) or (this->getBlowerState()));
}


void hot_tub::setOzone(uint8_t state)
{
  if (m_periph_ozone.state != state)
  {
    m_periph_ozone.state = state;
    this->ioWrite(m_periph_ozone.pin, m_periph_ozone.state);
  }
}

uint8_t hot_tub::getOzoneState()
{
  return m_periph_ozone.state;
}




void hot_tub::setLight(uint8_t state, uint8_t toggle)
{
  static uint8_t state_prv = 0;

  state_prv = m_periph_light.state;

  if (toggle)
    m_periph_light.state = !m_periph_light.state;
  else
    m_periph_ozone.state = state;

  if (state_prv != m_periph_light.state)
    this->ioWrite(m_periph_light.pin, m_periph_light.state);

}

uint8_t hot_tub::getLightState()
{
  return m_periph_light.state;
}
