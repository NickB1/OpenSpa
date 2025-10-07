
//WiFi, NTP settings
const uint8_t   openspa_wifi_enable =     1;
#define         openspa_timezone          TZ_Europe_Brussels
#define         openspa_ntp_servers       "europe.pool.ntp.org", "time.nist.gov"

//Maximum - minimum temperatures
const float openspa_init_desired_temp   = 38.0;
const float openspa_max_temperature     = 45.0;
const float openspa_min_temperature     = 5.0;

//Heating
const uint16_t  openspa_heating_timeout                 = 3600; //seconds
const float     openspa_heating_timeout_delta_degrees   = 1.0;
const uint16_t  openspa_heating_window_start_time       = 1600; //hours:minutes -- not implemented yet
const uint16_t  openspa_heating_window_stop_time        = 2300; //hours:minutes -- not implemented yet
const uint8_t   openspa_heating_pwm                     = 1;    //control heater power through PWM using external PCA9685 and e.g. Kemo M150 + Kemo M028N
const uint8_t   openspa_heating_pwm_pca9685_address     = 0x40;
const uint16_t  openspa_heating_min_power               = 500;

//Filtering/flush timing
const uint16_t openspa_filter_window_start_time  = 900;  //hours:minutes
const uint16_t openspa_filter_window_stop_time   = 2000; //hours:minutes
const uint16_t openspa_ozone_window_start_time   = 1100; //hours:minutes
const uint16_t openspa_ozone_window_stop_time    = 1600; //hours:minutes
const uint16_t openspa_filter_daily_cycles       = 4;
const uint16_t openspa_filter_time               = 3600;    //seconds

const uint16_t openspa_flush_window_start_time   = 1000; //hours:minutes
const uint16_t openspa_flush_window_stop_time    = 1800; //hours:minutes
const uint16_t openspa_flush_daily_cycles        = 3;
const uint16_t openspa_flush_time_pump_1         = 60;   //seconds
const uint16_t openspa_flush_time_pump_2         = 60;   //seconds
const uint16_t openspa_flush_time_blower         = 10;   //seconds

//Maximum run time and rest time for pumps and blower in seconds
const uint16_t openspa_runtime_pump_1      = 900;  //seconds
const uint16_t openspa_resttime_pump_1     = 60;   //seconds
const uint16_t openspa_runtime_pump_2      = 900;  //seconds
const uint16_t openspa_resttime_pump_2     = 60;   //seconds
const uint16_t openspa_runtime_blower      = 300;  //seconds
const uint16_t openspa_resttime_blower     = 60;   //seconds

//Power in Watts - not implemented yet
const uint16_t openspa_power_total_max     = 3600;
const uint16_t openspa_power_heater        = 2200;
const uint16_t openspa_power_pump_circ     = 400;
const uint16_t openspa_power_pump_1        = 1200;
const uint16_t openspa_power_pump_2        = 500;
const uint16_t openspa_power_blower        = 500;

//OneWire

const uint8_t openspa_onewire_temp_sensor_1_enable = 1;
const uint8_t openspa_onewire_temp_sensor_1_address[8] = {0x28, 0x45, 0x51, 0x13, 0x0C, 0x00, 0x00, 0xD3};

const uint8_t openspa_onewire_temp_sensor_2_enable = 1;
const uint8_t openspa_onewire_temp_sensor_2_address[8] = {0x28, 0x05, 0xBC, 0xAD, 0x3A, 0x19, 0x01, 0x1B};
