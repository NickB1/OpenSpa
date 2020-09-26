
const int homie_update_interval = 5;

HomieNode homieStatusNode       ("status"               , "Status"              , "status");
HomieNode homiePump1Node        ("pump1"                , "Pump 1"              , "switch");
HomieNode homiePump2Node        ("pump2"                , "Pump 2"              , "switch");
HomieNode homieBlowerNode       ("blower"               , "Blower"              , "switch");
HomieNode homieLightNode        ("light"                , "Light"               , "switch");
HomieNode homieTemperatureNode  ("temperature"          , "Temperature"         , "temperature");

uint8_t wifiInit()
{
  configTime(openspa_timezone, openspa_ntp_servers);

  Homie_setFirmware("OpenSpa", "1.0.0");
  Homie_setBrand("OpenSpa");
  Homie.setLoopFunction(homieLoopHandler);

  homieStatusNode.advertise("state").setDatatype("integer");
  homieStatusNode.advertise("error").setDatatype("integer");
  homiePump1Node.advertise("state").setDatatype("integer").settable(homiePump1Handler);
  homiePump2Node.advertise("state").setDatatype("boolean").settable(homiePump2Handler);
  homieBlowerNode.advertise("state").setDatatype("boolean").settable(homieBlowerHandler);
  homieLightNode.advertise("state").setDatatype("boolean").settable(homieLightHandler);
  homieTemperatureNode.advertise("current").setDatatype("float").setUnit("ºC");
  homieTemperatureNode.advertise("desired").setDatatype("float").setUnit("ºC").settable(homieTempHandler);
  
  Homie.disableLedFeedback();
  //Homie.setLedPin(16, HIGH);
  Homie.setup();
  
  return 0;
}

void homieLoopHandler()
{
  static unsigned long millis_prv = 0;
  
  if (millis() - millis_prv >= homie_update_interval * 1000UL || millis_prv == 0) 
  {
    homieStatusNode.setProperty("state").send(String(jacuzzi.getStatus()));
    homieStatusNode.setProperty("error").send(String(jacuzzi.getErrorCode()));
    homiePump1Node.setProperty("state").send(String(jacuzzi.getPump_1_State()));
    homiePump2Node.setProperty("state").send(String(jacuzzi.getPump_2_State() ? "true" : "false"));
    homieBlowerNode.setProperty("state").send(String(jacuzzi.getBlowerState() ? "true" : "false"));
    homieLightNode.setProperty("state").send(String(jacuzzi.getLightState() ? "true" : "false"));
    homieTemperatureNode.setProperty("current").send(String(jacuzzi.currentTemperature()));
    homieTemperatureNode.setProperty("desired").send(String(jacuzzi.desiredTemperature()));
    millis_prv = millis();
  }
}

bool homiePump1Handler(const HomieRange& range, const String& value)
{
  uint8_t setting = (value.toInt() & 0xFF);
  if((0 <= setting) & (setting <= 2))
  {
    jacuzzi.setPump_1(setting, 0);
    homiePump1Node.setProperty("state").send(value);
    return true;
  }
  return false;
}

bool homiePump2Handler(const HomieRange& range, const String& value)
{
  if(value == "true")
  {
    jacuzzi.setPump_2(true, 0);
    homiePump2Node.setProperty("state").send(value);
    return true;
  }
  else if(value == "false")
  {
    jacuzzi.setPump_2(false, 0);
    homiePump2Node.setProperty("state").send(value);
    return true;
  }
  return false;
}

bool homieBlowerHandler(const HomieRange& range, const String& value)
{
  if(value == "true")
  {
    jacuzzi.setBlower(true, 0);
    homieBlowerNode.setProperty("state").send(value);
    return true;
  }
  else if(value == "false")
  {
    jacuzzi.setBlower(false, 0);
    homieBlowerNode.setProperty("state").send(value);
    return true;
  }
  return false;
}

bool homieLightHandler(const HomieRange& range, const String& value)
{
  if(value == "true")
  {
    jacuzzi.setLight(true, 0);
    homieLightNode.setProperty("state").send(value);
    return true;
  }
  else if(value == "false")
  {
    jacuzzi.setLight(false, 0);
    homieLightNode.setProperty("state").send(value);
    return true;
  }
  return false;
}

bool homieTempHandler(const HomieRange& range, const String& value)
{
  if(jacuzzi.setDesiredTemperature(value.toFloat()))
    return false;
  else
  {
    homieTemperatureNode.setProperty("desired").send(value);
    return true;
  }
}

void wifiHandler(void)
{
  Homie.loop();
}

int timeOfDay()
{
  static int time_of_day = 0;
  time_t now;
  struct tm *timeinfo;

  now = time(nullptr);
  timeinfo = localtime(&now);
  time_of_day = ((timeinfo->tm_hour) * 100) + (timeinfo->tm_min);

  return time_of_day;
}
