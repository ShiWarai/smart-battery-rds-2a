#include "main.hpp"



void setup()
{
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Отключение детекции падения тока
	Serial.begin(CONFIG_CONSOLE_UART_BAUDRATE); // Временно до появление логов

	SemaphoreHandle_t wireMutex = xSemaphoreCreateMutex();

	SelfChecking::integrationTest();

	xTaskCreate(PreferencesController::preferencesTask, "Preferences task", 4096, NULL, 1, NULL);

	while(settings.battery_id == 0) // Ожидаем загрузки настроек в ОЗУ
		vTaskDelay(100);

	#ifdef WITH_DISPLAY
	xTaskCreate(DisplayController::displayTask, "Display task", 4096, wireMutex, 1, NULL);
	#endif
	xTaskCreate(SensorController::sensorTask, "Sensor task", 2048, wireMutex, 1, NULL);

	vTaskDelay(3000);

	xTaskCreate(UsbController::usbTask, "USB task", 4096, NULL, 2, NULL);
	xTaskCreate(WirelessController::wirelessTask, "Wireless task", 16384, NULL, 2, NULL);
}