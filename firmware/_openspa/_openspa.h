
//WiFi, NTP and MQTT settings
const uint8_t   openspa_wifi_enable =     1;
#define         openspa_wifi_ssid         "YOUR_WIFI_SSID"
#define         openspa_wifi_pass         "YOUR_WIFI_PASSWORD"
#define         openspa_timezone          TZ_Europe_Brussels


#define         openspa_ntp_servers       "europe.pool.ntp.org", "time.nist.gov"
#define         openspa_mqtt_broker_ip    "MQTT_BROKER_IP"
#define         openspa_mqtt_broker_port   1883 // use 8883 for SSL
#define         openspa_mqtt_username     "MQTT_USERNAME"
#define         openspa_mqtt_password     "MQTT_PASSWORD"

//Maximum - minimum temperatures
const float openspa_init_desired_temp   = 35.0;
const float openspa_max_temperature     = 40.0;
const float openspa_min_temperature     = 5.0;

//Heating
const uint16_t  openspa_heating_timeout                 = 3600; //seconds
const float     openspa_heating_timeout_delta_degrees   = 1.0;
const uint16_t  openspa_heating_window_start_time       = 1600; //hours:minutes -- not implemented yet
const uint16_t  openspa_heating_window_stop_time        = 2300; //hours:minutes -- not implemented yet

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
const uint16_t openspa_flush_time_blower         = 60;   //seconds

//Maximum run time and rest time for pumps and blower in seconds
const uint16_t openspa_runtime_pump_1      = 900;  //seconds
const uint16_t openspa_resttime_pump_1     = 60;   //seconds
const uint16_t openspa_runtime_pump_2      = 900;  //seconds
const uint16_t openspa_resttime_pump_2     = 60;   //seconds
const uint16_t openspa_runtime_blower      = 900;  //seconds
const uint16_t openspa_resttime_blower     = 60;   //seconds

//Power in Watts - not implemented yet
const uint16_t openspa_power_total_max     = 3600;
const uint16_t openspa_power_heater        = 2500;
const uint16_t openspa_power_pump_circ     = 400;
const uint16_t openspa_power_pump_1        = 1200;
const uint16_t openspa_power_pump_2        = 500;
const uint16_t openspa_power_blower        = 500;
