#pragma once

#include <U8g2lib.h>
#include <EncButton.h>
#include "sensor_controller/INA226Data.hpp"
#include "preferences_controller/settings.hpp"

#define CHANGE_MODE_HOLD_TIME 5000

class DisplayController
{
    public:
        static void displayTask(void *pvParameters);
    private:
        static void printStatus(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled, INA226Data data, bool changeContrast = false, byte contrast = 255);
        static void turnOnDisplay(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled);
        static void turnOffDisplay(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled);
        static void invertMode();
};