#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#ifndef WOKWI
#include <INA226.h>
#else
#include "sensor_controller\INA226_wokwi.hpp"
#endif
#include <WiFi.h>
#include <InfluxDbClient.h>
#include "self_checking/test_result.hpp"
#include "preferences_controller/settings.hpp"


class SelfChecking
{
public:
    static void integrationTest();
    static IntegrationTestResult getIntegrationTestResults();
private:
    inline static const char* TAG = "self_checking";
};