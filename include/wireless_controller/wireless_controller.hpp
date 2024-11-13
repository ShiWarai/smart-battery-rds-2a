#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <InfluxDbClient.h>
#include <ESPmDNS.h>
#include "preferences_controller/settings.hpp"
#include "sensor_controller/INA226Data.hpp"

#define TZ_INFO "UTC-3"

class WirelessController
{
public:
    static void wirelessTask(void *pvParameters);
private:
    static bool deserializeSettings(String json_str);
    static String serializeSettings();
    static String serializeTestingResult();
};