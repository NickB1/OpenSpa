
#include <ESP8266WiFi.h>
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

WiFiClient wifi_client;

uint8_t mqtt_update_rate_s = 5;
Adafruit_MQTT_Client mqtt(&wifi_client, openspa_mqtt_broker_ip, openspa_mqtt_broker_port, openspa_mqtt_username, openspa_mqtt_password);
Adafruit_MQTT_Publish     mqtt_openspa_status     = Adafruit_MQTT_Publish(&mqtt, openspa_mqtt_username "/openspa/status");
Adafruit_MQTT_Subscribe   mqtt_openspa_command    = Adafruit_MQTT_Subscribe(&mqtt, openspa_mqtt_username "/openspa/command");

uint8_t wifiInit()
{
  ESP.eraseConfig();

  configTime((3600 * openspa_timezone), (3600 * openspa_dst), openspa_ntp_servers);

  if (wifiConnect() == 0)
  {
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  if (ntpCheckSync() == 0)
    Serial.println("NTP time synced");

  mqtt.subscribe(&mqtt_openspa_command);

  if (mqttConnect() == 0)
    Serial.println("MQTT connected");

  Serial.println();
  return 0;
}

uint8_t wifiConnect()
{
  uint8_t timeout = 10;
  WiFi.mode(WIFI_STA);
  WiFi.begin(openspa_wifi_ssid, openspa_wifi_pass);

  while (timeout > 0)
  {
    if (WiFi.status() == WL_CONNECTED)
      return 0;

    delay(1000);
    timeout--;
  }
  return 1;
}

uint8_t ntpCheckSync()
{
  uint8_t timeout = 10;

  while (timeout > 0)
  {
    if (time(nullptr) >= 100000)
      return 0;

    delay(1000);
    timeout--;
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

void mqttHandler(void)
{
  static unsigned long time_now = 0, time_prv = 0;

  time_now = millis();

  if (mqtt.connected())
  {
    if ((time_now - time_prv) > (mqtt_update_rate_s * 1000))
    {
      time_prv = millis();

      Adafruit_MQTT_Subscribe *subscription;

      subscription = mqtt.readSubscription();
      
      if (subscription)
      {
        if (subscription == &mqtt_openspa_command)
        {
          //Serial.print(F("Command received: "));
          //Serial.println((char *)mqtt_openspa_command.lastread);
          jacuzzi.setDesiredTemperature(atof((char *)mqtt_openspa_command.lastread));
        }
      }

      char openspa_status_string[100];

      sprintf(openspa_status_string, "%d, %d, %d, %d, %.2f, %.2f, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d",
              openspa_error, timeOfDay(), jacuzzi.getErrorCode(), jacuzzi.getStatus(),
              jacuzzi.currentTemperature(), jacuzzi.desiredTemperature(), jacuzzi.getFilteringStatus(), jacuzzi.getHeatingStatus(),
              jacuzzi.getFilteringNextCycleTime(), jacuzzi.getFlushingNextCycleTime(), jacuzzi.getHeaterState(), jacuzzi.getCircPumpState(),
              jacuzzi.getPump_1_State(), jacuzzi.getPump_2_State(), jacuzzi.getBlowerState(), jacuzzi.getOzoneState(), jacuzzi.getLightState());

      mqtt_openspa_status.publish(openspa_status_string);

      // ping the server to keep the mqtt connection alive
      // NOT required if you are publishing once every KEEPALIVE seconds
      /*
        if(! mqtt.ping()) {
        mqtt.disconnect();
        }
      */
    }
  }
  else
  {
    if (mqtt.connect() != 0)
      mqtt.disconnect();
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
