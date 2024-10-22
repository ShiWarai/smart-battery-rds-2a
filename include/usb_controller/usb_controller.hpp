#pragma once

#include <Arduino.h>
#include <nvs_flash.h>
#include "preferences_controller/settings.hpp"
#include "sensor_controller/INA226Data.hpp"

class UsbController
{
public:
    static void usbTask(void *pvParameters);
private:
    static void clearInputBuffer();
    static void com_menu();
    static void settings_menu();
    static int inputRead();
};