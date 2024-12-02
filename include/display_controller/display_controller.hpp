#pragma once

#include <U8g2lib.h>
#include <algorithm>
#include <EncButton.h>
#include <WiFi.h>
#include "sensor_controller/INA226Data.hpp"
#include "preferences_controller/settings.hpp"

#define CHANGE_MODE_HOLD_TIME 5000

enum SCREEN_MODE {
    NONE,
    MAIN,
    WIFI,
    CHART,
    COUNT
};

class DisplayController
{
    public:
        static void displayTask(void *pvParameters);
    private:
        static void displayScreen(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled, SCREEN_MODE screen_mode);

        static void printHistoryMenu(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled, INA226Data data, bool scale, bool changeContrast = false, byte contrast = 255);
        static void printSecondMenu(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled, INA226Data data, bool changeContrast = false, byte contrast = 255);
        static void printStatus(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled, INA226Data data, bool changeContrast = false, byte contrast = 255);
        static void turnOnDisplay(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled);
        static void turnOffDisplay(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled);
        static void invertMode();
};