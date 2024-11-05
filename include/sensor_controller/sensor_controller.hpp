#pragma once

#ifndef WOKWI
#include <INA226.h>
#else
#include "INA226_wokwi.hpp"
#endif

#include "INA226Data.hpp"
#include "preferences_controller/settings.hpp"

class SensorController
{
public:
    static void sensorTask(void *pvParameters);
};