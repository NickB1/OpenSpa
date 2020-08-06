
const uint16_t display_prompt_time_ms = 3000;
char error_text[5];

void displayHandler(void)
{
  bbDisplayHandler();
}

void bbDisplayHandler(void)
{
  static uint8_t display_prompt = 0;
  static unsigned long time_prv = 0;

  if (display_prompt == 0)
  {
    if (openspa_error)
    {
      snprintf(error_text, sizeof(error_text), "%04d", openspa_error);
      error_text[0] = 'E';
      bb_display.printText(error_text);
    }
    else
    {
      if((jacuzzi.currentTemperature() < (jacuzzi.desiredTemperature() - 0.5)) or (jacuzzi.currentTemperature() > (jacuzzi.desiredTemperature() + 0.5)))
        bb_display.printDegrees(jacuzzi.currentTemperature(), false);
      else
        bb_display.printDegrees(jacuzzi.desiredTemperature(), false);
    }
  }
  else
  {
    if ((millis() - time_prv) > display_prompt_time_ms)
    {
      display_prompt = 0;
    }
  }

  if (bb_display.poll()) //Polls display and returns pressed buttons
  {
    time_prv = millis();
    switch (bb_display.readButtonData())
    {
      case balboa_display_button_temp_up:
        jacuzzi.increaseDesiredTemperature();
        bb_display.printDegrees(jacuzzi.desiredTemperature(), (jacuzzi.desiredTemperature() > jacuzzi.currentTemperature()));
        display_prompt = 1;
        break;

      case balboa_display_button_temp_down:
        jacuzzi.decreaseDesiredTemperature();
        bb_display.printDegrees(jacuzzi.desiredTemperature(), (jacuzzi.desiredTemperature() > jacuzzi.currentTemperature()));
        display_prompt = 1;
        break;

      case balboa_display_button_lights:
        jacuzzi.setLight(false, true); //Toggle lights
        break;

      case balboa_display_button_jets:
        jacuzzi.setPump_1(false, true); //Toggle jets
        break;

      case balboa_display_button_blower:
        jacuzzi.setBlower(false, true); //Toggle blower
        break;

      case balboa_display_button_hidden:

        break;

      default:

        break;
    }
  }

  //Set display lighs according to peripheral status
  bb_display.setLightHeat(jacuzzi.getHeaterState());
  bb_display.setLightJets(jacuzzi.getPump_1_State());
  bb_display.setLightBlower(jacuzzi.getBlowerState());
}
