#pragma once

#include <Arduino.h>

#include <preferences_controller/preferences_controller.hpp>
#ifdef WITH_TEST
#include <test_mode/test_mode.hpp>
#endif
#include <sensor_controller/sensor_controller.hpp>
#include <wireless_controller/wireless_controller.hpp>
#include <usb_controller/usb_controller.hpp>
#ifdef WITH_DISPLAY
#include <display_controller/display_controller.hpp>
#endif

// Functions
void setup();
void loop() {vTaskDelete(NULL);}