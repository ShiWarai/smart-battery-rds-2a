#include "main.hpp"

void setup()
{
	SemaphoreHandle_t wireMutex = xSemaphoreCreateMutex();

	xTaskCreate(PreferencesController::preferencesTask, "Preferences task", 4096, NULL, 1, NULL);

	while(settings.battery_id == 0) // Ожидаем загрузки настроек в ОЗУ
		vTaskDelay(100); 

	#ifdef WITH_DISPLAY
	xTaskCreate(DisplayController::displayTask, "Display task", 4096, wireMutex, 1, NULL);
	#endif
	xTaskCreate(SensorController::sensorTask, "Sensor task", 2048, wireMutex, 1, NULL);
	xTaskCreate(UsbController::usbTask, "USB task", 2048, NULL, 1, NULL);
	xTaskCreate(WirelessController::wirelessTask, "Wireless task", 8196, NULL, 1, NULL);
}