
const uint8_t console_print_status      = 's';
const uint8_t console_print_commands    = 'c';

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
  Serial.println(timeOfDay());
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
  Serial.print("Heating status: ");
  Serial.println(jacuzzi.getHeatingStatus());
  Serial.print("Filter next cycle time: ");
  Serial.println(jacuzzi.getFilteringNextCycleTime());
  Serial.print("Flush next cycle time: ");
  Serial.println(jacuzzi.getFlushingNextCycleTime());
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
  Serial.println();
  Serial.println("WiFi");
  Serial.println("________________");
  Serial.print("Status: ");
  Serial.println(WiFi.status());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("WiFi Reconnects: ");
  Serial.print("WiFi RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println("dBm");
  Serial.println(wifi_reconnects);
  Serial.print("MQTT Reconnects: ");
  Serial.println(mqtt_reconnects);
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

      default:
        break;
    }
  }
}
