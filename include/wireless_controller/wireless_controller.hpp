#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <InfluxDbClient.h>
#include <ESPmDNS.h>
#include "preferences_controller/settings.hpp"
#include "sensor_controller/INA226Data.hpp"
#include "self_checking/self_checking.hpp"
#include "united_control/united_control.hpp"

class WirelessController
{
public:
    static void wirelessTask(void *pvParameters);
private:
    static bool deserializeSettings(String json_str, bool &needReboot);
    static String serializeSettings();
    static String serializeTestingResult();
};