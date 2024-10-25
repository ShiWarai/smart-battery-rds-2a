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
    static void settingsMenu();
    static void test();

    static String readInput();
    static String readString(error_t (String));
    static uint32_t readUInt32(error_t (String));
    static float readFloat(error_t (String));
};