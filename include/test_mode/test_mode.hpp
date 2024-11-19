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
#include "test_mode/integration_test_result.hpp"
#include "preferences_controller/settings.hpp"


class TestMode
{
public:
    static void test();
    static IntegrationTestResult getTestResults();
};