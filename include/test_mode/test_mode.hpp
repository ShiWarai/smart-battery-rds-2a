#pragma once

#include <Arduino.h>
// дисплей
#include <U8g2lib.h>
// INA
#ifndef WOKWI
#include <INA226.h>
#else
#include "sensor_controller\INA226_wokwi.hpp"
#endif
// wi-fi
#include <WiFi.h>
#include <ESPmDNS.h>


#include <Preferences.h>

class TestMode
{
public:
    static void test();
private:
    static bool testlog[];
};