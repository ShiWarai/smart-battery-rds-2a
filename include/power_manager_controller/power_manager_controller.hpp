#pragma once

#include "Arduino.h"  //CpuFrequency
#include "driver/adc.h" //ADC


struct POWERMODE{
    uint32_t CpuFrequencyMhz;
};

class PowerManagerController
{
public:
    static void switchingToSleepMode();
private:
    
};