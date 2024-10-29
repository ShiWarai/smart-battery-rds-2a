#pragma once

// дисплей
#include <U8g2lib.h>
// INA
#ifndef WOKWI
#include <INA226.h>
#else
#include "sensor_controller\INA226_wokwi.hpp"
#endif
// 


#include "preferences_controller/settings.hpp"

class TestMode
{
public:
    static void test();
private:
    static bool testlog[];
};