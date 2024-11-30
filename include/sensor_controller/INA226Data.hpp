#pragma once

#ifndef WOKWI
#include <INA226.h>
#else
#include "INA226_wokwi.hpp"
#endif
#ifdef MS_MEASUREMENTS_ENABLE
#include <esp_timer.h>
#endif
#include <ArduinoJson.h>
#include <deque>
#include "preferences_controller/settings.hpp"

#define FLOAT_MAP(value, in_min, in_max, out_min, out_max) ((value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)
#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))

class INA226Data
{
public:
	uint32_t *id;
	float voltage;
	float current;
	float power;
	double energy; // в ватт-часах
	float capacity;
	time_t timestamp;
	uint64_t timestamp_ms;

	std::deque<float> voltageBuffer;
	std::deque<float> currentBuffer;
	std::deque<float> powerBuffer;
	std::deque<float> capacityBuffer;
	std::deque<time_t> timestampBuffer;
	std::deque<uint64_t> timestampMsBuffer;

	INA226Data(uint32_t *id, float startVoltage)
	{
		this->id = id;
		this->energy = FLOAT_MAP(startVoltage, 3.0, 4.2, 0.0, MAX_ENERGY_CAPACITY); // Примерно
	}

	void readData(INA226 *sensor)
	{
		this->voltage = sensor->getBusVoltage();
		this->current = sensor->getCurrent();
		this->power = sensor->getPower() * SIGN(this->current);
		this->energy -= ((double)this->power * settings.sensor_delay / 3600000);
		this->capacity = FLOAT_MAP(this->energy, 0, MAX_ENERGY_CAPACITY, 0.0, 100.0);
		this->timestamp = time(nullptr);
		#ifdef MS_MEASUREMENTS_ENABLE
		struct timeval tv_now;
		gettimeofday(&tv_now, NULL);
		this->timestamp_ms = (uint64_t)tv_now.tv_sec * 1000L + tv_now.tv_usec / 1000;
		#endif

		// Добавляем новые значения в буфер и удаляем старые
		updateBuffer(voltageBuffer, this->voltage);
		updateBuffer(currentBuffer, this->current);
		updateBuffer(powerBuffer, this->power);
		updateBuffer(capacityBuffer, this->capacity);
		updateBuffer(timestampBuffer, this->timestamp);
		updateBuffer(timestampMsBuffer, this->timestamp_ms);
	};

	String getJSON(size_t N = 1)
	{
		JsonDocument json;

		if (N > METRICS_BUFFER_SIZE)
		{
			N = METRICS_BUFFER_SIZE;
		}

		// Заполняем JSON-документ
		json["ID"] = *(this->id);
		if(N == 1) {
			json["V"] = this->voltage;
			json["A"] = this->current;
			json["P"] = this->power;
			json["C"] = this->capacity;
			#ifndef MS_MEASUREMENTS_ENABLE
			json["T"] = this->timestamp;
			#else
			json["t"] = this->timestamp_ms;
			#endif
		} else {
			getLastNMetrics(json, "V", voltageBuffer, N);
			getLastNMetrics(json, "A", currentBuffer, N);
			getLastNMetrics(json, "P", powerBuffer, N);
			getLastNMetrics(json, "C", capacityBuffer, N);
			#ifndef MS_MEASUREMENTS_ENABLE
			getLastNMetrics(json, "T", timestampBuffer, N);
			#else
			getLastNMetrics(json, "t", timestampMsBuffer, N);
			#endif
		}

		serializeJson(json, buffer);
		return buffer;
	};

private:
	String buffer;

	template <typename T>
	void updateBuffer(std::deque<T> & buffer, T newValue)
	{
		if (buffer.size() >= METRICS_BUFFER_SIZE)
			buffer.pop_front();
		
		buffer.push_back(newValue);
	}

	template <typename T>
	JsonArray getLastNMetrics(JsonDocument &json, const char* name, std::deque<T> &deque_buffer, size_t n)
	{
		JsonArray arr = json[name].to<JsonArray>();
		
		if (deque_buffer.size() <= n)
		{
			for (const T &val : deque_buffer)
				arr.add(val);
		}
		else
		{
			auto start = deque_buffer.end() - n;
			for (auto it = start; it != deque_buffer.end(); ++it)
				arr.add(*it);
		}

		return arr;
	}
};

inline INA226Data *raw_data = nullptr;
