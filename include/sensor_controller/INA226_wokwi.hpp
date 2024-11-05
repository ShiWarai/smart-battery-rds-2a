#pragma once

#include "Arduino.h"
#include <Wire.h>
#include <cstdlib>
#include <ctime>
#include <cmath>

class INA226 {
public:
    //  address between 0x40 and 0x4F
    explicit INA226(const uint8_t address, TwoWire *wire = &Wire) {
        std::srand(static_cast<unsigned int>(std::time(0)));
        this->_address = address;
        this->_wire = wire;
    }

    bool begin() {
        this->connected = true;
        return true;
    }

    bool isConnected() {
        return this->connected;
    }

    uint8_t getAddress() {
        return this->_address;
    }

    //  Core functions
    float getBusVoltage() {
        return generateSinusoidalValue(3.9f, 4.0f);
    }

    float getShuntVoltage() {
        return getBusVoltage() - 0.1;
    }

    float getCurrent() {
        return generateCosinusoidalValue(0.5f, 1.0f);
    }

    float getPower() {
        return getBusVoltage() * getCurrent();
    }

    //  See #35
    bool isConversionReady() {
        return true;  // Dummy implementation for now
    }

    bool waitConversionReady(uint32_t timeout = 10) {
        // Dummy implementation
        return true;
    }

    //  Scale helpers milli range
    float getBusVoltage_mV() {
        return getBusVoltage() * 1e3;
    }

    float getShuntVoltage_mV() {
        return getShuntVoltage() * 1e3;
    }

    float getCurrent_mA() {
        return getCurrent() * 1e3;
    }

    float getPower_mW() {
        return getPower() * 1e3;
    }

    //  Scale helpers micro range
    float getBusVoltage_uV() {
        return getBusVoltage() * 1e6;
    }

    float getShuntVoltage_uV() {
        return getShuntVoltage() * 1e6;
    }

    float getCurrent_uA() {
        return getCurrent() * 1e6;
    }

    float getPower_uW() {
        return getPower() * 1e6;
    }

    //  Configuration
    bool reset() {
        return true;  // Dummy reset
    }

    bool setAverage(uint8_t avg = 10) {
        return true;  // Dummy implementation
    }

    uint8_t getAverage() {
        return 10;  // Dummy implementation
    }

    bool setBusVoltageConversionTime(uint8_t bvct = 10) {
        return true;  // Dummy implementation
    }

    uint8_t getBusVoltageConversionTime() {
        return 10;  // Dummy implementation
    }

    bool setShuntVoltageConversionTime(uint8_t svct = 10) {
        return true;  // Dummy implementation
    }

    uint8_t getShuntVoltageConversionTime() {
        return 10;  // Dummy implementation
    }

    //  Calibration
    int setMaxCurrentShunt(float maxCurrent = 20.0, float shunt = 0.002, bool normalize = true) {
        this->_maxCurrent = maxCurrent;
        this->_shunt = shunt;
        this->_current_LSB = 1.0;
        return 1;
    }

    bool isCalibrated() {
        return _current_LSB != 0.0;
    }

    float getCurrentLSB() {
        return _current_LSB;
    }

    float getCurrentLSB_mA() {
        return _current_LSB * 1e3;
    }

    float getCurrentLSB_uA() {
        return _current_LSB * 1e6;
    }

    float getShunt() {
        return _shunt;
    }

    float getMaxCurrent() {
        return _maxCurrent;
    }

    //  Operating mode
    bool setMode(uint8_t mode = 7) {
        return true;  // Dummy implementation
    }

    uint8_t getMode() {
        return 7;  // Dummy implementation
    }

    bool shutDown() {
        return setMode(0);
    }

    bool setModeShuntTrigger() {
        return setMode(1);
    }

    bool setModeBusTrigger() {
        return setMode(2);
    }

    bool setModeShuntBusTrigger() {
        return setMode(3);
    }

    bool setModeShuntContinuous() {
        return setMode(5);
    }

    bool setModeBusContinuous() {
        return setMode(6);
    }

    bool setModeShuntBusContinuous() {
        return setMode(7);
    }

    //  Alert
    bool setAlertRegister(uint16_t mask) {
        return true;  // Dummy implementation
    }

    uint16_t getAlertFlag() {
        return 0;  // Dummy implementation
    }

    bool setAlertLimit(uint16_t limit) {
        return true;  // Dummy implementation
    }

    uint16_t getAlertLimit() {
        return 0;  // Dummy implementation
    }

    //  Meta information
    uint16_t getManufacturerID() {
        return 0x5449;
    }

    uint16_t getDieID() {
        return 0x2260;
    }

    //  DEBUG
    uint16_t getRegister(uint8_t reg) {
        return _readRegister(reg);
    }

private:
    uint16_t _readRegister(uint8_t reg) {
        return 0;  // Dummy implementation
    }

    uint16_t _writeRegister(uint8_t reg, uint16_t value) {
        return 0;  // Dummy implementation
    }

    float generateSinusoidalValue(float min_v, float max_v) {
        float amplitude = (max_v - min_v) / 2.0;
        float offset = min_v + amplitude;
        float radians = static_cast<float>(sin_generator_counter) * M_PI / 50.0;
        sin_generator_counter = (sin_generator_counter + 1) % MAX_COUNTER;
        return offset + amplitude * sin(radians);
    }

    float generateCosinusoidalValue(float min_v, float max_v) {
        float amplitude = (max_v - min_v) / 2.0;
        float offset = min_v + amplitude;
        float radians = static_cast<float>(cos_generator_counter) * M_PI / 50.0;
        cos_generator_counter = (cos_generator_counter + 1) % MAX_COUNTER;
        return offset + amplitude * cos(radians);
    }

    // Переменные

    float _current_LSB;
    float _shunt;
    float _maxCurrent;
    uint8_t _address;
    TwoWire *_wire;
    bool connected = false;

    const int MAX_COUNTER = 100;
    int sin_generator_counter = 0;
    int cos_generator_counter = 0;
};
