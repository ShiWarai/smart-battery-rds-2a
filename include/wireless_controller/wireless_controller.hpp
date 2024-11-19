#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <InfluxDbClient.h>
#include <ESPmDNS.h>
#include "preferences_controller/settings.hpp"
#include "sensor_controller/INA226Data.hpp"
#include "test_mode/integration_test_result.hpp"
#include "united_control/united_control.hpp"

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