#include <math.h>

const int     thermistor_adc_resolution   = pow(2, 10);
const float   thermistor_ref_voltage      = 5.087;
const long    thermistor_series_resistor  = 10000;
const uint8_t thermistor_average_samples  = 10;

const long    thermistor_nominal_value    = 30000;
const int     thermistor_nominal_temp     = 25;
const int     thermistor_b_coefficient    = 3762;


float thermistorRead()
{
  // 1/3.2 resistor divider on Wemos, 1x buffer on OpenSpa board

  float series_resistor_voltage = (analogRead(A0) * (3.20 / thermistor_adc_resolution));
  float thermistor_value = ((thermistor_ref_voltage - series_resistor_voltage) / (series_resistor_voltage / thermistor_series_resistor));

  //return (-24.31185 * log(thermistor_value)) + 277.46527;

  float steinhart;
  steinhart = thermistor_value / thermistor_nominal_value;     // (R/Ro)
  steinhart = log(steinhart);                   // ln(R/Ro)
  steinhart /= thermistor_b_coefficient;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (thermistor_nominal_temp + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C

  return 0.5 * round(2.0 * steinhart); //Round to half integer
}
