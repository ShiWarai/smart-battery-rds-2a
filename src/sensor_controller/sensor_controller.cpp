#include "sensor_controller/sensor_controller.hpp"

void SensorController::sensorTask(void *pvParameters) {
	
	SemaphoreHandle_t wireMutex = static_cast<SemaphoreHandle_t>(pvParameters);

	INA226 INA = INA226(0x40);

    raw_data = new INA226Data(&settings.battery_id);

	if (!INA.begin() && Serial.isConnected())
		Serial.println("it was not possible to connect to the voltampermeter. Fix the error");
	else
		INA.setMaxCurrentShunt(25, 0.00125 * settings.shunt_mult_res, true);

	while(true) {
		if (xSemaphoreTake(wireMutex, portMAX_DELAY) == pdTRUE)
		{
			raw_data->readData(&INA);

			xSemaphoreGive(wireMutex);
		}

		vTaskDelay(settings.sensor_delay);
	}
}