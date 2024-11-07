#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <InfluxDbClient.h>
#include "preferences_controller/settings.hpp"
#include "sensor_controller/INA226Data.hpp"

class WirelessController
{
public:
    static void wirelessTask(void *pvParameters);
private:
    static bool deserializeSettings(String json_str);
    static String serializeSettings();
    static String serializeTestingResult();
};