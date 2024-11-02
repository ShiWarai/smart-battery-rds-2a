#pragma once

#include <Arduino.h>
#include <nvs_flash.h>
#include "preferences_controller/settings.hpp"
#include "sensor_controller/INA226Data.hpp"
#include "usb_controller/validators.hpp"

#define DECLARE_SERIAL_PRINT_ITER(TYPE, F1, F2, SETTING_POINTER) \
if(SETTING_TYPES[i] == #TYPE) \
{ \
    Serial.print(SETTING_NAMES[i]); \
    Serial.print("\r\t\t\t="); \
    Serial.print(*(std::get<TYPE*>(SETTING_POINTER))); \
}

#define GEN_SETTINGS_OUTPUT_DEFAULT(SETTING_POINTER, TYPES) \
for (unsigned short i = 0; i < SETTING_TYPE::SETTINGS_COUNT; i++) { \
    SETTING_POINTER = getSettingFieldPointer(i); \
    Serial.print(String(i+1)+") "); \
    TYPES(DECLARE_SERIAL_PRINT_ITER, SETTING_POINTER) \
    Serial.println(); \
}

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
    static error_t read_String(String *, error_t validator(String));
    static error_t read_uint32_t(uint32_t *, error_t validator(String));
    static error_t read_float(float *, error_t validator(String));
};