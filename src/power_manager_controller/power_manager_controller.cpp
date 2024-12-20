#include "power_manager_controller/power_manager_controller.hpp"

void PowerManagerController::switchingToSleepMode(){
    setCpuFrequencyMhz(240);
    adc_power_off();
}