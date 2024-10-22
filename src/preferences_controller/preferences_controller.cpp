#include "preferences_controller/preferences_controller.hpp"

void PreferencesController::preferencesTask(void *pvParameters) {
    
	Preferences preferences;
	SettingUpdate update;
	
	preferences.begin(SETTINGS_SPACE_NAME, false);

	// Инициализация дефолтных значений
	settings.battery_id = ID;
	settings.wifi_password = WIFI_PASSWORD;
	settings.wifi_ssid = WIFI_SSID;
	settings.wireless_delay = 1000;
	settings.usb_delay = 1000;
	settings.sensor_delay = 1000;
	settings.access_key = DEFAULT_ACCESS_KEY;

	if(!preferences.isKey(SETTING_NAMES[SETTING_TYPE::battery_id])) { // Первичная инициализация настроек
		preferences.putUInt(SETTING_NAMES[SETTING_TYPE::battery_id], settings.battery_id);
		preferences.putString(SETTING_NAMES[SETTING_TYPE::access_key], settings.access_key);
		preferences.putString(SETTING_NAMES[SETTING_TYPE::wifi_ssid], settings.wifi_ssid);
		preferences.putString(SETTING_NAMES[SETTING_TYPE::wifi_password], settings.wifi_password);
		preferences.putUInt(SETTING_NAMES[SETTING_TYPE::sensor_delay], settings.sensor_delay);
		preferences.putUInt(SETTING_NAMES[SETTING_TYPE::usb_delay], settings.usb_delay);
		preferences.putUInt(SETTING_NAMES[SETTING_TYPE::wireless_delay], settings.wireless_delay);
	}

	DECLARE_SETTING_TYPES_LINKS_VARIANT(UNIQUE_SETTINGS_TYPES) setting;
	void* buffer;

	// Загрузка настроек
	if(preferences.isKey(SETTING_NAMES[SETTING_TYPE::battery_id])) {
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