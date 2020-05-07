
#include <string.h>
#include <ESP8266WiFi.h>
#include <TZ.h>
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

uint16_t  wifi_reconnects = 0;
uint16_t  mqtt_reconnects = 0;
