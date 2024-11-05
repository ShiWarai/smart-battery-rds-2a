#pragma once


#ifndef WOKWI
#include <INA226.h>
#else
#include "INA226_wokwi.hpp"
#endif

#include <ArduinoJson.h>

#define FLOAT_MAP(value, in_min, in_max, out_min, out_max) ((value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)
#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))


class INA226Data {
public:
	INA226Data(uint32_t* id) {
		this->id = id;
	};
	
	void readData(INA226 *sensor) {
		this->voltage = sensor->getBusVoltage();
		this->current = sensor->getCurrent();
		this->power = sensor->getPower() * SIGN(this->current);
		this->capacity = FLOAT_MAP(sensor->getBusVoltage(),3.3,4.2,0.0,100.0);
	};

	// String getJSON() {
	// 	return "{\"ID\":" + String(this->id) + ", \"V\":" + this->voltage + ", \"A\":" + this->current + ", \"P\":" + this->power + ", \"C\":" + String(this->capacity) + "}";
	// };

	String getJSON() {
		// Заполняем JSON-документ
		json["ID"] = *(this->id);
		json["V"] = round2(this->voltage);
		json["A"] = round2(this->current);
		json["P"] = round2(this->power);
		json["C"] = round2(this->capacity);

		// Конвертируем JSON-документ в строку
		serializeJson(json, buffer);
		return buffer;
	};

	uint32_t *id;
	float voltage;
	float current;
	float power;
	float capacity;

private:
	JsonDocument json;
	String buffer;

	float round2(float value) {
   		return (int)(value * 100 + 0.5) / 100.0;
	}
};

inline INA226Data *raw_data = nullptr;