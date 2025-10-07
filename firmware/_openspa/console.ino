
const uint8_t console_print_status      = 's';
const uint8_t console_print_commands    = 'c';
const uint8_t console_reset             = 'r';

uint8_t consoleInit()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("OpenSpa.be v1.00 by Nick Blommen");
  Serial.println();
  return 0;
}

void consolePrintCommands()
{
  Serial.println("Commands");
  Serial.println("________________________________");
  Serial.println("r: Reset");
  Serial.println("s: Print Status");
  Serial.println("c: Print Commands");
  Serial.println();
}

void consolePrintStatus()
{
  time_t now = time(nullptr);

  Serial.println("OpenSpa");
  Serial.println("________________________________");
  Serial.print("Error code: ");
  Serial.println(openspa_error);
  Serial.print("Time: ");
  Serial.print(ctime(&now));
  Serial.print("Time of day: ");
  Serial.printf("%04d",timeOfDay());
  Serial.println();
  Serial.println();
  Serial.println("Hot Tub");
  Serial.println("________________");
  Serial.print("Error code: ");
  Serial.println(jacuzzi.getErrorCode());
  Serial.print("Status: ");
  Serial.println(jacuzzi.getStatus());
  Serial.print("Temperature desired: ");
  Serial.println(jacuzzi.desiredTemperature(), 2);
  Serial.print("Temperature: ");
  Serial.println(jacuzzi.currentTemperature(), 2);
  Serial.print("Temperature max: ");
  Serial.println(jacuzzi.maxTemperature(), 2);
  Serial.print("Filter status: ");
  Serial.println(jacuzzi.getFilteringStatus());
  Serial.print("Heating enabled status: ");
  Serial.println(jacuzzi.getHeatingEnabledState());
  Serial.print("Heating status: ");
  Serial.println(jacuzzi.getHeatingStatus());
  Serial.print("Heating power: ");
  Serial.println(jacuzzi.getHeatingPower());
  Serial.print("Heating PWM duty cycle: ");
  Serial.println(jacuzzi.getHeatingPwmDutyCycle());
  Serial.print("Filter next cycle time: ");
  Serial.printf("%04d",jacuzzi.getFilteringNextCycleTime());
  Serial.println();
  Serial.print("Flush next cycle time: ");
  Serial.printf("%04d",jacuzzi.getFlushingNextCycleTime());
  Serial.println();
  Serial.print("Heater state: ");
  Serial.println(jacuzzi.getHeaterState());
  Serial.print("Circulation pump state: ");
  Serial.println(jacuzzi.getCircPumpState());
  Serial.print("Pump 1 state: ");
  Serial.println(jacuzzi.getPump_1_State());
  Serial.print("Pump 2 state: ");
  Serial.println(jacuzzi.getPump_2_State());
  Serial.print("Blower state: ");
  Serial.println(jacuzzi.getBlowerState());
  Serial.print("Ozone state: ");
  Serial.println(jacuzzi.getOzoneState());
  Serial.print("Light state: ");
  Serial.println(jacuzzi.getLightState());
  Serial.print("Pressure switch state: ");
  Serial.println(jacuzzi.getPressureSwitchState());
  Serial.println();
  Serial.println("OneWire");
  Serial.println("________________");
  Serial.print("Internal Temperature: ");
  Serial.println(ds18s20_1.getTemperature());
  Serial.print("Jet Pump Temperature: ");
  Serial.println(ds18s20_2.getTemperature());
  Serial.println();
  Serial.println("WiFi");
  Serial.println("________________");
  Serial.print("Status: ");
  Serial.println(WiFi.status());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("WiFi RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println("dBm");
  Serial.println();


  
}

void consoleHandler()
{
  if (Serial.available())
  {
    int inByte = Serial.read();
    switch (inByte)
    {
      case console_print_status:
        consolePrintStatus();
        break;

      case console_print_commands:
        consolePrintCommands();
        break;

      case console_reset:
        Serial.println("Resetting");
        ESP.reset();
        break;

      default:
        break;
    }
  }
}
