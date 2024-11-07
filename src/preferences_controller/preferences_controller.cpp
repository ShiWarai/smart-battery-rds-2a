#include "preferences_controller/preferences_controller.hpp"

void PreferencesController::preferencesTask(void *pvParameters) {
    
	Preferences preferences;
	SettingUpdate update;
	
	preferences.begin(SETTINGS_SPACE_NAME, false);

	// Инициализация дефолтных значений
	settings.battery_id = ID;
	settings.wifi_password = WIFI_PASSWORD;
	settings.wifi_ssid = WIFI_SSID;
	settings.wireless_delay = 1000 / portTICK_PERIOD_MS;
	settings.usb_delay = 1000 / portTICK_PERIOD_MS;
	settings.sensor_delay = 1000 / portTICK_PERIOD_MS;
	settings.display_time = 10000 / portTICK_PERIOD_MS;
	settings.mode = 0;
	settings.db_host = DB_HOSTNAME;
	settings.db_port = DB_PORT;
	settings.hostname = HOSTNAME;
	settings.access_key = DEFAULT_ACCESS_KEY;

	DECLARE_SETTING_TYPES_LINKS_VARIANT(UNIQUE_SETTINGS_TYPES) setting;
	void* buffer;

	if(!preferences.isKey(SETTING_NAMES[SETTING_TYPE::battery_id])) { // Первичная инициализация настроек
		GEN_PUTS_DEFAULT(preferences, buffer, setting, UNIQUE_SETTINGS_TYPES)
	} else { // Загрузка настроек
		GEN_READ_SETTINGS_CYCLE(preferences, buffer, setting, UNIQUE_SETTINGS_TYPES)
	}

	preferences.end();

	while(true) {
		if (xQueueReceive(settingUpdateQueue, &update, portMAX_DELAY)) {
			preferences.begin(SETTINGS_SPACE_NAME, false);
			GEN_UPDATE_ITER(preferences, buffer, setting, update, UNIQUE_SETTINGS_TYPES)
			preferences.end();
		}

		vTaskDelay(100);
	}
}