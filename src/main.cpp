#include "main.hpp"

void setup()
{
	SemaphoreHandle_t wireMutex = xSemaphoreCreateMutex();

	SelfChecking::integrationTest();

	xTaskCreate(PreferencesController::preferencesTask, "Preferences task", 4096, NULL, 1, NULL);

	while(settings.battery_id == 0) vTaskDelay(100); // Ожидаем загрузки настроек в ОЗУ

	#ifdef WITH_DISPLAY
	xTaskCreate(DisplayController::displayTask, "Display task", 4096, wireMutex, 1, NULL);
	#endif
	xTaskCreate(SensorController::sensorTask, "Sensor task", 2048, wireMutex, 1, NULL);

	vTaskDelay(3000);

	xTaskCreate(UsbController::usbTask, "USB task", 4096, NULL, 2, NULL);
	xTaskCreate(WirelessController::wirelessTask, "Wireless task", 16384, NULL, 2, NULL);
}