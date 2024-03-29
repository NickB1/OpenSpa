
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

  reset();
}


//External functions

uint8_t hot_tub::ioWrite(uint8_t pin, uint8_t state)
{
  ioExpanderWrite(pin, state);
  //return 0;
  return (!(ioExpanderRead(pin) == state));
}

uint8_t hot_tub::ioRead(uint8_t pin)
{
  return ioExpanderRead(pin);
}

uint8_t hot_tub::setPWM(uint8_t duty_cycle)
{
  pwmWrite(duty_cycle);
  return 0;
}

void hot_tub::readTemp()
{
  static unsigned long timestamp = 0;
  if ((timePassed(timestamp)) > 1) //Sample every second
  {
    m_current_temperature = thermistorRead();
    if (m_current_temperature > m_max_temperature)
    {
      m_max_temperature = m_current_temperature;
    }
    timestamp = timeStamp();
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
  setHeater(false);
  setCircPump(false);
  setPump_1(false, false);
  setPump_2(false, false);
  setBlower(false, false);
  setLight(false, false);
  setOzone(false);

  m_heating_enabled = true;

  m_status = 0;
  m_error_code = 0;

  filter_heater_state_machine(true); //Reset state machine
}

void hot_tub::allOff()
{
  setHeater(false);
  setCircPump(false);
  setPump_1(false, false);
  setPump_2(false, false);
  setBlower(false, false);
  setLight(false, false);
  setOzone(false);
}



uint8_t hot_tub::poll()
{
  uint8_t out_of_use = 0;

  if (hot_tub_debug == 0)
  {
    errorChecking();
  }

  readTemp();
  checkRuntime();

  if (m_error_code == 0)
  {
    out_of_use = (outOfUse());
    filtering(out_of_use, out_of_use); //Force filter cycle whitout ozone when out of use
    heating();
    filter_heater_state_machine(false);
    flushing();
  }
  else
    allOff();

  outputController();

  return m_error_code;
}



uint8_t hot_tub::outOfUse()
{
  const uint16_t min_usage_time_s = 300;
  static unsigned long timestamp = 0;
  static uint8_t state_prv = 0;
  uint8_t out_of_use = 0;

  if ((getPumpsAndBlowerState() != 0) and (state_prv == 0)) //In use
    timestamp = timeStamp();
  else if ((getPumpsAndBlowerState() == 0) and (state_prv != 0)) //Out of use
  {
    if ((timePassed(timestamp) > min_usage_time_s))
    {
      out_of_use = 1;
    }
  }

  state_prv = getPumpsAndBlowerState();
  return out_of_use;
}



uint8_t hot_tub::errorChecking()
{
  static unsigned long timestamp = 0;
  if ((timePassed(timestamp)) > 2) //Sample every 2 seconds
  {
    //Max Temperature
    if (m_current_temperature > m_max_temperature_limit)
    {
      m_error_code = hot_tub_error_overtemp;
    }

    //Temp sensor
    if ((m_current_temperature < 0) && (hot_tub_debug == 0))
    {
      m_error_code = hot_tub_error_temp_sensor;
    }

    //Heater
    if (getHeaterState())
    {
      //Heater on without circulation pump
      if (getCircPumpState() == false)
      {
        m_error_code = hot_tub_error_heating_on_without_circ_pump;
      }

      //Heater timeout
      //Heating with pwm control causes longer heating times, so best to not do this check when enabled
      if (((timePassed(m_periph_heater.timestamp)) > m_heating_timeout) && (!m_heating_pwm_enabled))
      {
        if (m_current_temperature < (m_heating_initial_temperature + m_heating_timeout_delta_degrees))
        {
          m_error_code = hot_tub_error_heating_timeout;
        }
      }
    }
    timestamp = timeStamp();
  }

  //Pressure switch always on
  if (getCircPumpState() == false)
  {
    if ((timePassed(m_periph_circ_pump.timestamp)) > m_pressure_switch_max_delay_s)
    {
      if (getPressureSwitchState() == true)
      {
        m_error_code = hot_tub_error_pressure_switch_always_on;
      }
    }
  }

  //Pressure switch failed
  if (getCircPumpState() == true)
  {
    if ((timePassed(m_periph_circ_pump.timestamp)) > m_pressure_switch_max_delay_s)
    {
      if (getPressureSwitchState() == false)
      {
        m_error_code = hot_tub_error_pressure_switch_failed;
      }
    }
  }

  return m_error_code;
}

void hot_tub::outputController()
{
  static unsigned long timestamp = 0;
  static uint8_t main_state = 0;
  uint8_t peripheral_state = 0;

  peripheral_state = (getHeaterState() |
                      getCircPumpState() |
                      getPump_1_State() |
                      getPump_2_State() |
                      getBlowerState() |
                      getOzoneState());

  switch (main_state)
  {
    case 0: //Set main relay when a peripheral is set
      if (peripheral_state != 0)
      {
        timestamp = timeStamp();
        setMain(true);
        setMainOutput();
        main_state = 1;
      }
      setHeaterPWM_ON_OFF(false);
      break;

    case 1: //Wait
      if ((timePassed(timestamp)) > m_main_output_on_delay_s)
      {
        main_state = 2;
        timestamp = timeStamp();
      }
      break;

    case 2: //Set peripheral relais, stay here as long as a certain peripheral is set

      if (m_heating_pwm_enabled)
      {
        if (getHeaterState())
        {
          if ((timePassed(timestamp)) > m_main_output_on_delay_s)
            setHeaterPWM_ON_OFF(true);
        }
        else
        {
          setHeaterPWM_ON_OFF(false);
          timestamp = timeStamp();
        }
      }

      setMainOutput();
      setHeaterOutput();
      setCircPumpOutput();
      setPump_1_Output();
      setPump_2_Output();
      setBlowerOutput();
      setOzoneOutput();

      if (peripheral_state == 0)
      {
        timestamp = timeStamp();
        main_state = 3;
      }
      break;

    case 3: //Wait m_main_output_off_delay_s to turn main relay off when all peripherals are off, jump back to previous state if a peripheral is set
      if ((timePassed(timestamp)) > m_main_output_off_delay_s)
      {
        setMain(false);
        setMainOutput();
        main_state = 0;
      }
      else if (peripheral_state != 0)
      {
        main_state = 2;
      }
      break;

    default:
      main_state = 0;
      break;
  }
}


void hot_tub::setFilteringSettings(uint16_t filter_window_start_time, uint16_t filter_window_stop_time, uint16_t ozone_window_start_time,
                                   uint16_t ozone_window_stop_time, uint16_t filter_daily_cycles, uint16_t filter_time_s)
{
  m_filter_window_start_time = filter_window_start_time;
  m_filter_window_stop_time = filter_window_stop_time;
  m_ozone_window_start_time = ozone_window_start_time;
  m_ozone_window_stop_time = ozone_window_stop_time;
  m_filter_daily_cycles = filter_daily_cycles;
  m_filter_time_s = filter_time_s;
}

void hot_tub::setHeatingSettings(uint8_t heating_pwm, uint16_t heating_min_power, uint16_t heating_timeout, float heating_timeout_delta_degrees)
{
  m_heating_pwm_enabled = heating_pwm;
  m_heating_min_power = heating_min_power;
  m_heating_timeout = heating_timeout;
  m_heating_timeout_delta_degrees = heating_timeout_delta_degrees;
}

void hot_tub::setFlushingSettings(uint16_t flush_window_start_time, uint16_t flush_window_stop_time, uint16_t flush_daily_cycles,
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

  next_cycle = (getTimeOfDay() + cycle_period);
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
  static uint16_t cycle_period = getCyclePeriod(m_filter_window_start_time, m_filter_window_stop_time, m_filter_time_s, m_filter_daily_cycles);
  static uint8_t filter_cycle = 0;

  time_of_day = getTimeOfDay();

  if (force_filter_cycle)
  {
    filtering_state = start_filtering;
    if (force_no_ozone == 1)
      m_filtering_ozone_enabled = false;
    else
      m_filtering_ozone_enabled = true;
  }

  switch (filtering_state)
  {
    case idle:

      if ((m_filter_window_start_time <= time_of_day) and (m_filter_window_stop_time >= time_of_day))
      {
        if (time_of_day == m_filter_next_cycle_time)
          filtering_state = start_filtering;
      }
      else
      {
        m_filter_next_cycle_time = m_filter_window_start_time;
      }

      if ((m_ozone_window_start_time < time_of_day) and (m_ozone_window_stop_time > time_of_day))
        m_filtering_ozone_enabled = true;
      else
        m_filtering_ozone_enabled = false;

      break;

    case start_filtering:
      m_filter_next_cycle_time = getNextCycleTime(cycle_period);
      if ((m_filter_next_cycle_time < m_filter_window_start_time) or (m_filter_next_cycle_time > m_filter_window_stop_time))
      {
        m_filter_next_cycle_time = m_filter_window_start_time;
      }
      timestamp = timeStamp();
      filter_cycle++;
      m_filtering_run = true;
      filtering_state = filtering;
      break;

    case filtering:
      if ((timePassed(timestamp)) >= m_filter_time_s) //check if done
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
  static unsigned long timestamp = 0;
  if (m_heating_enabled | (hot_tub_debug == 1))
  {
    if (timePassed(timestamp) >= m_heating_holdoff_time)
    {
      if ((m_current_temperature - m_desired_temperature) <=  m_desired_temperature_delta_minus)
      {
        timestamp = timeStamp();
        m_heating_run = true;
      }
      else if ((m_current_temperature - m_desired_temperature) >=  m_desired_temperature_delta_plus)
      {
        timestamp = timeStamp();
        m_heating_run = false;
      }
    }
  }
  else
  {
    m_heating_run = false;
  }
}

void hot_tub::filter_heater_state_machine(uint8_t reset)
{
  enum enum_filter_heater_state { idle, start_pump, filtering, start_heater, heating, pause, unpause};
  static enum_filter_heater_state filter_heater_state = pause;
  static unsigned long timestamp = 0;

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
        if ((getPumpsAndBlowerState()) == 0) //only start when all pumps and blower are turned off
          filter_heater_state = start_pump;
      }

      setCircPump(false);
      setHeater(false);
      setOzone(false);

      break;

    case start_pump:
      if (getPumpsAndBlowerState()) //Pause when a pump or blower is enabled
        filter_heater_state = pause;

      setCircPump(true);

      if (m_filtering_ozone_enabled)
        setOzone(true);

      if (m_heating_run == true)
      {
        filter_heater_state = start_heater;
        timestamp = timeStamp();
      }
      else if (m_filtering_run == true)
        filter_heater_state = filtering;

      break;

    case filtering:
      if (getPumpsAndBlowerState()) //Pause when a pump or blower is enabled
        filter_heater_state = pause;

      m_status = hot_tub_status_filtering;

      if (m_filtering_run == false)
      {
        filter_heater_state = idle;
        setCircPump(false);
        setOzone(false);
      }
      else if (m_heating_run == true)
      {
        filter_heater_state = start_heater;
        timestamp = timeStamp();
      }

      break;

    case start_heater:
      if (getPumpsAndBlowerState()) //Pause when a pump or blower is enabled
        filter_heater_state = pause;

      if ((timePassed(timestamp)) >= m_filter_heater_state_delay_s)
      {
        setHeater(true);
        filter_heater_state = heating;
        timestamp = timeStamp();
      }
      break;

    case heating:
      if (getPumpsAndBlowerState()) //Pause when a pump or blower is enabled
        filter_heater_state = pause;

      if (m_filtering_run)
        m_status = hot_tub_status_filtering_heating;
      else
        m_status = hot_tub_status_heating;

      if ((timePassed(timestamp)) >= m_filter_heater_state_delay_s)
      {
        if (m_heating_run == false)
        {
          setHeater(false);
          filter_heater_state = filtering;
        }
      }

      break;

    case pause:
      setCircPump(false);
      setHeater(false);
      setOzone(false);

      if (getPumpsAndBlowerState() == 0) //only start back up when all pumps and blower are turned off
      {
        timestamp = timeStamp();
        filter_heater_state = unpause;
      }
      break;

    case unpause:
      if (getPumpsAndBlowerState()) //Pause when a pump or blower is enabled
        filter_heater_state = pause;

      if ((timePassed(timestamp)) > m_filter_heater_unpause_delay_s)
        filter_heater_state = idle;
      break;

    default:
      setCircPump(false);
      setHeater(false);
      setOzone(false);
      filter_heater_state = idle;
      break;
  }
}

void hot_tub::flushing()
{
  uint16_t time_of_day = 0;
  static unsigned long timestamp = 0;
  enum enum_flushing_state { idle, start_flushing, start_blower, flushing_blower, start_pump_1, flushing_pump_1, start_pump_2,  flushing_pump_2 };
  static enum_flushing_state flushing_state = start_flushing;
  static uint16_t cycle_period = getCyclePeriod(m_flush_window_start_time, m_flush_window_stop_time, m_flush_time_pump_1_s, m_flush_daily_cycles);
  static uint8_t flush_cycle = 0;
  uint8_t flush_inter_delay_s = 3;
  static uint8_t pumps_and_blower_state_prv = 0, flushing_started = 0;

  time_of_day = getTimeOfDay();

  if ((m_flush_window_start_time <= time_of_day) and (m_flush_window_stop_time >= time_of_day))
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
          m_flush_next_cycle_time = getNextCycleTime(cycle_period);
        }
        else
        {
          if (time_of_day == m_flush_next_cycle_time)
            flushing_state = start_flushing;
        }
        m_flushing_run = 0;
        break;

      case start_flushing:
        m_flush_next_cycle_time = getNextCycleTime(cycle_period);
        if (m_flush_next_cycle_time > m_flush_window_stop_time)
        {
          m_flush_next_cycle_time = m_flush_window_start_time;
        }
        timestamp = timeStamp();
        flush_cycle++;
        flushing_state = start_blower;
        m_flushing_run = 1;
        break;

      case start_blower:
        if ((timePassed(timestamp)) >= flush_inter_delay_s)
        {
          timestamp = timeStamp();
          if (m_flush_time_blower_s > 0)
          {
            setBlower(true, false);
            flushing_state = flushing_blower;
          }
          else
          {
            flushing_state = start_pump_1;
          }
        }
        break;

      case flushing_blower:
        if ((timePassed(timestamp)) >= m_flush_time_blower_s)
        {
          timestamp = timeStamp();
          setBlower(false, false);
          flushing_state = start_pump_1;
        }
        break;

      case start_pump_1:
        if ((timePassed(timestamp)) >= flush_inter_delay_s)
        {
          timestamp = timeStamp();
          if (m_flush_time_pump_1_s > 0)
          {
            setPump_1(true, false);
            flushing_state = flushing_pump_1;
          }
          else
          {
            flushing_state = start_pump_2;
          }

        }
        break;

      case flushing_pump_1:
        if ((timePassed(timestamp)) >= m_flush_time_pump_1_s)
        {
          timestamp = timeStamp();
          setPump_1(false, false);
          flushing_state = start_pump_2;
        }
        break;

      case start_pump_2:
        if ((timePassed(timestamp)) >= flush_inter_delay_s)
        {
          timestamp = timeStamp();
          if (m_flush_time_pump_2_s > 0)
          {
            setPump_2(true, false);
            flushing_state = flushing_pump_2;
          }
          else
          {
            flushing_state = idle;
          }
        }
        break;

      case flushing_pump_2:
        if ((timePassed(timestamp)) >= m_flush_time_pump_2_s)
        {
          timestamp = timeStamp();
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
  if (getPump_1_State() and ((timePassed(m_periph_pump_1.timestamp)) >= m_periph_pump_1.runtime))
    setPump_1(false, false);

  if (getPump_2_State() and ((timePassed(m_periph_pump_2.timestamp)) >= m_periph_pump_2.runtime))
    setPump_2(false, false);

  if (getBlowerState() and ((timePassed(m_periph_blower.timestamp)) >= m_periph_blower.runtime))
    setBlower(false, false);
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

uint8_t hot_tub::getFlushingStatus()
{
  return m_flushing_run;
}

uint16_t hot_tub::getFlushingNextCycleTime()
{
  return m_flush_next_cycle_time;
}




void hot_tub::setMinTemperature(float min_temperature)
{
  m_min_temperature_limit = min_temperature;
}

void hot_tub::setMaxTemperature(float max_temperature)
{
  m_max_temperature_limit = max_temperature;
}

bool hot_tub::setDesiredTemperature(float desired_temperature)
{
  if ((desired_temperature <= m_max_temperature_limit) && (desired_temperature >= m_min_temperature_limit))
  {
    m_desired_temperature = desired_temperature;
    return false;
  }
  return true;
}

void hot_tub::increaseDesiredTemperature()
{
  m_desired_temperature += m_desired_temperature_increment;
  if (m_desired_temperature > m_max_temperature_limit)
    m_desired_temperature = m_max_temperature_limit;
}

void hot_tub::decreaseDesiredTemperature()
{
  m_desired_temperature -= m_desired_temperature_increment;
  if (m_desired_temperature < m_min_temperature_limit)
    m_desired_temperature = m_min_temperature_limit;
}

float hot_tub::currentTemperature()
{
  readTemp();
  return m_current_temperature;
}

float hot_tub::desiredTemperature()
{
  return m_desired_temperature;
}

float hot_tub::maxTemperature()
{
  return m_max_temperature;
}




void hot_tub::setMaxTotalPower(uint16_t power)
{
  m_max_total_power = power;
}


void hot_tub::setMain(uint8_t state)
{
  m_periph_main.state = state;
  m_periph_main.timestamp = timeStamp();
}

void hot_tub::setMainOutput()
{
  if (m_periph_main.output_state != m_periph_main.state)
  {
    if (ioWrite(m_periph_main.pin, m_periph_main.state) == 0)
      m_periph_main.output_state =  m_periph_main.state;
  }
}




void hot_tub::setHeatingEnabled(uint8_t state)
{
  if (state == true)
    m_heating_enabled = true;
  else
    m_heating_enabled = false;
}

uint8_t hot_tub::getHeatingEnabledState()
{
  return m_heating_enabled;
}

void hot_tub::setHeatingPower(uint16_t power)
{
  if (power <= m_periph_heater.power)
  {
    if (power <= m_heating_min_power)
      m_heating_power = m_heating_min_power;
    else
      m_heating_power = power;
  }
  else
    m_heating_power = m_periph_heater.power;
}

uint16_t hot_tub::getHeatingPower()
{
  return m_heating_power;
}

uint8_t hot_tub::getHeatingPwmDutyCycle()
{
  return m_current_pwm_duty_cycle;
}

void hot_tub::setHeaterPower(uint16_t power)
{
  m_periph_heater.power = power;
  m_heating_power = power; //Set max heating power initially
}

void hot_tub::setHeaterPWM_ON_OFF(uint8_t state)
{
  if (state)
  {
    //Formula: duty_cycle = (relative_power * 41) + 15 - Kemo M150 + M028N transfer function

    float relative_power = ((float) m_heating_power / (float) m_periph_heater.power);
    uint8_t duty_cycle = (int) ((relative_power * 41) + 15);

    if (duty_cycle >= 60) //max power starts at 60% duty cycle
      duty_cycle = 100;
      

    if (m_current_pwm_duty_cycle != duty_cycle)
    {
      if (hot_tub::setPWM(duty_cycle) == 0)
      {
        m_current_pwm_duty_cycle = duty_cycle;
      }
    }
  }
  else
  {
    if (m_current_pwm_duty_cycle != 0)
    {
      if (hot_tub::setPWM(0) == 0)
      {
        m_current_pwm_duty_cycle = 0;
      }
    }
  }
}

void hot_tub::setHeater(uint8_t state)
{
  m_periph_heater.state = state;
  m_periph_heater.timestamp = timeStamp();
}

void hot_tub::setHeaterOutput()
{
  if (m_periph_heater.output_state != m_periph_heater.state)
  {
    if (ioWrite(m_periph_heater.pin, m_periph_heater.state) == 0)
      m_periph_heater.output_state = m_periph_heater.state;

    m_heating_initial_temperature = m_current_temperature;
  }
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
  m_periph_circ_pump.state = state;
  m_periph_circ_pump.timestamp = timeStamp();
}

void hot_tub::setCircPumpOutput()
{
  if (m_periph_circ_pump.output_state != m_periph_circ_pump.state)
  {
    if (ioWrite(m_periph_circ_pump.pin, m_periph_circ_pump.state) == 0)
      m_periph_circ_pump.output_state = m_periph_circ_pump.state;
  }
}

uint8_t hot_tub::getCircPumpState()
{
  return m_periph_circ_pump.state;
}


uint8_t hot_tub::getPressureSwitchState()
{
  return (!(ioRead(m_pin_pressure_switch))); //Active low
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

  m_periph_pump_1.timestamp = timeStamp();
}

void hot_tub::setPump_1_Output()
{
  if (m_periph_pump_1.output_state != m_periph_pump_1.state)
  {
    if ((ioWrite(m_periph_pump_1.pin, (((m_periph_pump_1.state) | (m_periph_pump_1.state >> 1)) & 0x01)) == 0) &&
        (ioWrite(m_periph_pump_1.pin_speed, ((m_periph_pump_1.state >> 1) & 0x01)) == 0))
      m_periph_pump_1.output_state = m_periph_pump_1.state;
  }
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
  if (toggle)
    m_periph_pump_2.state = !m_periph_pump_2.state;
  else
    m_periph_pump_2.state = state;

  m_periph_pump_2.timestamp = timeStamp();
}

void hot_tub::setPump_2_Output()
{
  if (m_periph_pump_2.output_state != m_periph_pump_2.state)
  {
    if (ioWrite(m_periph_pump_2.pin, m_periph_pump_2.state) == 0)
      m_periph_pump_2.output_state = m_periph_pump_2.state;
  }
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
  if (toggle)
    m_periph_blower.state = !m_periph_blower.state;
  else
    m_periph_blower.state = state;

  m_periph_blower.timestamp = timeStamp();
}

void hot_tub::setBlowerOutput()
{
  if (m_periph_blower.output_state != m_periph_blower.state)
  {
    if (ioWrite(m_periph_blower.pin, m_periph_blower.state) == 0)
      m_periph_blower.output_state = m_periph_blower.state;
  }
}

uint8_t hot_tub::getBlowerState()
{
  return m_periph_blower.state;
}

uint8_t hot_tub::getPumpsAndBlowerState()
{
  return ((getPump_1_State()) or (getPump_2_State()) or (getBlowerState()));
}


void hot_tub::setOzone(uint8_t state)
{
  m_periph_ozone.state = state;
}

void hot_tub::setOzoneOutput()
{
  if (m_periph_ozone.output_state != m_periph_ozone.state)
  {
    if (ioWrite(m_periph_ozone.pin, m_periph_ozone.state) == 0)
      m_periph_ozone.output_state = m_periph_ozone.state;
  }
}

uint8_t hot_tub::getOzoneState()
{
  return m_periph_ozone.state;
}




void hot_tub::setLight(uint8_t state, uint8_t toggle)
{
  uint8_t state_new = 0;

  if (toggle)
    state_new = !m_periph_light.state;
  else
    state_new = state;

  if (state_new != m_periph_light.state)
  {
    if (ioWrite(m_periph_light.pin, state_new) == 0)
      m_periph_light.state = state_new;
  }

}

uint8_t hot_tub::getLightState()
{
  return m_periph_light.state;
}
