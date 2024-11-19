#pragma once

#include <WiFi.h>
#include "sensor_controller/INA226Data.hpp"
#include "preferences_controller/settings.hpp"
#include "test_mode/test_mode.hpp"

class UnitedControl
{
 public:
    static void writeToMemory(std::variant<String, float, uint32_t, nullptr_t> BUFFER, SETTING_TYPE NAME);
    static void restartSystem();
    static void startTest(bool needRestart=true);
    static IntegrationTestResult readTestResults();
 private:
};