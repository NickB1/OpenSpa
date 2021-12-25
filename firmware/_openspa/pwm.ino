
uint8_t pwmInit()
{
  pwm_driver.begin();
  pwm_driver.setOscillatorFrequency(27000000);
  pwm_driver.setPWMFreq(1500);  // This is the maximum PWM frequency

  for (int i = 0; i <= 15; i++) //Reset outputs
    pwm_driver.setPin(i, 0, false);

  return 0;
}

uint8_t pwmWrite(uint8_t duty_cycle)
{
  if (duty_cycle >= 100)
    pwm_driver.setPin(0, 4095, false);
  else
    pwm_driver.setPin(0, 40.95 * duty_cycle, false);

  return 0;
}
