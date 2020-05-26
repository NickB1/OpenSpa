
WiFiClient wifi_client;

uint8_t   wifi_reconnect_holdoff_s = 10;
uint8_t   mqtt_update_rate_s = 5;

Adafruit_MQTT_Client      mqtt(&wifi_client, openspa_mqtt_broker_ip, openspa_mqtt_broker_port, openspa_mqtt_username, openspa_mqtt_password);
Adafruit_MQTT_Publish     mqtt_pub_openspa_status     = Adafruit_MQTT_Publish(&mqtt, openspa_mqtt_username "/openspa/status");
Adafruit_MQTT_Subscribe   mqtt_sub_openspa_command    = Adafruit_MQTT_Subscribe(&mqtt, openspa_mqtt_username "/openspa/command");

uint8_t wifiInit()
{
  ESP.eraseConfig();

  configTime(openspa_timezone, openspa_ntp_servers);
  //configTime((3600 * openspa_timezone), (3600 * openspa_dst), openspa_ntp_servers);

  if (wifiConnect() == 0)
  {
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  if (ntpCheckSync() == 0)
    Serial.println("NTP time synced");

  mqtt.subscribe(&mqtt_sub_openspa_command);

  if (mqttConnect() == 0)
    Serial.println("MQTT connected");

  Serial.println();
  return 0;
}

uint8_t wifiConnect()
{
  uint8_t timeout_s = 10;
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(openspa_wifi_ssid, openspa_wifi_pass);

  //WiFi.setAutoConnect(true);
  //WiFi.setAutoReconnect(true);

  while (timeout_s > 0)
  {
    if (WiFi.status() == WL_CONNECTED)
      return 0;

    delay(1000);
    timeout_s--;
  }
  return 1;
}

uint8_t wifiReconnect()
{
  const uint8_t timeout_s = 10;
  static uint8_t state = 0;
  static unsigned long timestamp = 0;

  if (WiFi.status() == WL_CONNECTED)
  {
    state = 0;
    timestamp = 0;
    return 0;
  }
  else
  {
    if (state == 0)
    {
      wifi_reconnects++;
      WiFi.disconnect();
      WiFi.begin(openspa_wifi_ssid, openspa_wifi_pass);
      timestamp = millis();
      state = 1;
      Serial.println("WiFi reconnect");
    }
    else
    {
      if ((millis() - timestamp) > (timeout_s * 1000))
      {
        state = 0;
      }
    }
  }
  return 1;
}

uint8_t ntpCheckSync()
{
  uint8_t timeout_s = 10;

  while (timeout_s > 0)
  {
    if (time(nullptr) >= 100000)
      return 0;

    delay(1000);
    timeout_s--;
  }
  return 1;
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
uint8_t mqttConnect()
{
  // Stop if already connected.
  if (mqtt.connected()) {
    return 0;
  }

  uint8_t retries = 3;
  while (mqtt.connect() != 0) { // connect will return 0 for connected
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      return 1;
    }
  }
  return 0;
}

void mqttSubscription(void)
{
  static Adafruit_MQTT_Subscribe *subscription;

  subscription = mqtt.readSubscription();

  if (subscription)
  {
    if (subscription == &mqtt_sub_openspa_command)
    {
      //Serial.print(F("Command received: "));
      //Serial.println((char *)mqtt_sub_openspa_command.lastread);
      //if (strcmp((char *)mqtt_sub_openspa_command.lastread,"reset"))
      //{
      //  openspaReset();
      //}
      //else
      //{
      jacuzzi.setDesiredTemperature(atof((char *)mqtt_sub_openspa_command.lastread));
      //}
    }
  }
}

void mqttPublish(void)
{
  char openspa_status_string[100];

  sprintf(openspa_status_string, "%d, %04d, %d, %d, %.2f, %.2f, %.2f, %d, %d, %04d, %04d, %d, %d, %d, %d, %d, %d, %d, debug: wifi_rssi:%d, wifi_r:%d, mqtt_r:%d",
          openspa_error, timeOfDay(), jacuzzi.getErrorCode(), jacuzzi.getStatus(),
          jacuzzi.desiredTemperature(), jacuzzi.currentTemperature(), jacuzzi.maxTemperature(), jacuzzi.getFilteringStatus(), jacuzzi.getHeatingStatus(),
          jacuzzi.getFilteringNextCycleTime(), jacuzzi.getFlushingNextCycleTime(), jacuzzi.getHeaterState(), jacuzzi.getCircPumpState(),
          jacuzzi.getPump_1_State(), jacuzzi.getPump_2_State(), jacuzzi.getBlowerState(), jacuzzi.getOzoneState(), jacuzzi.getLightState(),
          WiFi.RSSI(), wifi_reconnects, mqtt_reconnects);

  mqtt_pub_openspa_status.publish(openspa_status_string);
}

void wifiHandler(void)
{
  static unsigned long time_now = 0, time_prv = 0;
  static uint8_t mqtt_cnt_holfoff = 0;

  time_now = millis();

  if (WiFi.status() == WL_CONNECTED)
  {
    if (mqtt.connected())
    {
      mqtt_cnt_holfoff = 0;

      if ((time_now - time_prv) > (mqtt_update_rate_s * 1000))
      {
        time_prv = millis();

        mqttSubscription();
        mqttPublish();
      }
    }
    else
    {
      if (mqtt_cnt_holfoff == 0)
      {
        mqtt_reconnects++;
        mqtt_cnt_holfoff = 1;
      }

      if (mqtt.connect() != 0)
        mqtt.disconnect();
    }
  }
  else
  {
    wifiReconnect();
  }
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
