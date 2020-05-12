
const uint8_t hot_tub_debug = 1;

#define hot_tub_status_idle                           0
#define hot_tub_status_in_use                         1
#define hot_tub_status_filtering                      2
#define hot_tub_status_heating                        3
#define hot_tub_status_filtering_heating              4

#define hot_tub_error_overtemp                        1
#define hot_tub_error_pressure_switch_always_on       2
#define hot_tub_error_pressure_switch_failed          3
#define hot_tub_error_heating_timeout                 4
#define hot_tub_error_heating_on_without_circ_pump    5
#define hot_tub_error_temp_sensor                     6


class hot_tub
{
  public:
    hot_tub(uint8_t pin_o_main, uint8_t pin_o_heater, uint8_t pin_o_circulation_pump,
            uint8_t pin_o_pump_1, uint8_t pin_o_pump_1_speed, uint8_t pin_o_pump_2,
            uint8_t pin_o_blower, uint8_t pin_o_ozone_generator, uint8_t pin_o_light,
            uint8_t pin_i_pressure_switch);

    void reset();
    void allOff();
    uint8_t poll();

    void setMinTemperature(float min_temperature);
    void setMaxTemperature(float max_temperature);
    void setDesiredTemperature(float desired_temperature);
    void increaseDesiredTemperature();
    void decreaseDesiredTemperature();
    float currentTemperature();
    float desiredTemperature();
    float maxTemperature();

    void setMaxTotalPower(uint16_t power);
    void setHeaterPower(uint16_t power);
    void setCircPumpPower(uint16_t power);
    void setPump_1_Power(uint16_t power);
    void setPump_2_Power(uint16_t power);
    void setBlowerPower(uint16_t power);

    void setPump_1(uint8_t state, uint8_t toggle); //2 speed, stage = 0x01 -> low, stage = 0x02 -> high
    void setPump_2(uint8_t state, uint8_t toggle);
    void setBlower(uint8_t state, uint8_t toggle);
    void setLight(uint8_t state, uint8_t toggle);

    void setPump_1_Timing(uint16_t runtime, uint16_t resttime);
    void setPump_2_Timing(uint16_t runtime, uint16_t resttime);
    void setBlowerTiming(uint16_t runtime, uint16_t resttime);

    void setFiltering(uint16_t filter_window_start_time, uint16_t filter_window_stop_time, uint16_t ozone_window_start_time,
                      uint16_t ozone_window_stop_time, uint16_t filter_daily_cycles, uint16_t filter_time_s);
    void setHeating(uint16_t heating_timeout, float heating_timeout_delta_degrees);
    void setFlushing(uint16_t m_flush_window_start_time, uint16_t m_flush_window_stop_time, uint16_t flush_daily_cycles,
                     uint16_t flush_time_pump_1_s, uint16_t flush_time_pump_2_s, uint16_t flush_time_blower_s);

    uint8_t getStatus();
    uint8_t getErrorCode();
    uint8_t getFilteringStatus();
    uint16_t getFilteringNextCycleTime();
    uint8_t getHeatingStatus();
    uint8_t getFlushingStatus();
    uint16_t getFlushingNextCycleTime();

    uint8_t getHeaterState();
    uint8_t getCircPumpState();
    uint8_t getPump_1_State();
    uint8_t getPump_2_State();
    uint8_t getBlowerState();
    uint8_t getOzoneState();
    uint8_t getLightState();

  private:

    struct peripheral
    {
      uint8_t pin, pin_speed;
      uint8_t state;
      uint16_t power;
      unsigned long timestamp;
      uint16_t runtime;
      uint16_t resttime;
    };

    uint8_t m_status;
    uint8_t m_error_code;

    peripheral m_periph_main;
    peripheral m_periph_heater;
    peripheral m_periph_circ_pump;
    peripheral m_periph_pump_1;
    peripheral m_periph_pump_2;
    peripheral m_periph_blower;
    peripheral m_periph_ozone;
    peripheral m_periph_light;
    uint8_t m_pin_pressure_switch;

    float m_min_temperature_limit = 5.0;
    float m_max_temperature_limit = 38.0;
    float m_current_temperature = 35.0;
    float m_desired_temperature = 35.0;
    float m_heating_initial_temperature = 35.0;
    float m_max_temperature = 0.0;
    const float m_desired_temperature_increment = 0.5;
    const float m_desired_temperature_delta_plus = 0.5; //Heating OFF
    const float m_desired_temperature_delta_minus = -0.5; //Heating ON

    const uint16_t m_filter_heater_state_delay_s = 5;
    const uint16_t m_filter_heater_unpause_delay_s = 10;
    uint16_t m_heating_holdoff_time         = 60; //seconds
    uint16_t m_heating_timeout              = 3600; //seconds
    float m_heating_timeout_delta_degrees   = 1.0;

    uint16_t m_filter_window_start_time  = 900;  //hours:minutes
    uint16_t m_filter_window_stop_time   = 2000; //hours:minutes
    uint16_t m_ozone_window_start_time   = 1000; //hours:minutes
    uint16_t m_ozone_window_stop_time    = 1800; //hours:minutes
    uint16_t m_filter_daily_cycles       = 4;
    uint16_t m_filter_time_s             = 3600; //seconds

    uint16_t m_filter_next_cycle_time    = 0; //calculated by filtering function

    uint16_t m_flush_window_start_time   = 1000; //hours:minutes
    uint16_t m_flush_window_stop_time    = 1800; //hours:minutes
    uint16_t m_flush_daily_cycles        = 6;
    uint16_t m_flush_time_pump_1_s       = 60;
    uint16_t m_flush_time_pump_2_s       = 60;
    uint16_t m_flush_time_blower_s       = 60;

    uint16_t m_flush_next_cycle_time    = 0; //calculated by flushing function

    uint16_t m_max_total_power = 0;
    uint16_t m_total_power = 0;
    uint8_t m_filtering_run = 0, m_filtering_ozone_enabled = 0, m_heating_run = 0, m_flushing_run = 0;

    const uint16_t m_pressure_switch_max_delay_s = 5; //Delay in seconds for the pressure switch to enable after turning on circ pump

    void ioWrite(uint8_t pin, uint8_t state);
    uint8_t ioRead(uint8_t pin);
    void readTemp();
    unsigned long timeStamp();
    unsigned long timePassed(unsigned long timestamp);
    uint16_t getTimeOfDay();
    uint16_t getTimeDiffDecimalHours(uint16_t time_start, uint16_t time_stop);
    uint16_t getCyclePeriod(uint16_t cycle_window_start_time, uint16_t cycle_window_stop_time, uint16_t cycle_time_s, uint16_t cycles);
    uint16_t getNextCycleTime(uint16_t cycle_period);

    uint8_t errorChecking();
    void filtering(uint8_t force_filter_cycle, uint8_t force_no_ozone);
    void heating();
    void filter_heater_state_machine(uint8_t reset);
    void flushing();

    void checkRuntime();

    void setHeater(uint8_t state);
    void setCircPump(uint8_t state);
    void setOzone(uint8_t state);

    uint8_t getPumpsAndBlowerState();
    uint8_t getPressureSwitchState();
    uint8_t outOfUse();
};
