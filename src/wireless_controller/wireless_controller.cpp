#include "wireless_controller/wireless_controller.hpp"

#define DECLARE_DESERIALIZE_ITER(TYPE, NAME, SOURCE_STR, JSON_NAME, SETTINGS_NAME, UPDATE_QUEUE) \
SettingUpdate update_##NAME; \
if(json[#NAME].is<TYPE>()) { \
	update_##NAME.key = SETTING_TYPE::NAME; \
	update_##NAME.value = JSON_NAME[#NAME].as<TYPE>(); \
	xQueueSend(UPDATE_QUEUE, &update_##NAME, portMAX_DELAY); \
}

#define DECLARE_SERIALIZE_ITER(TYPE, NAME, JSON_NAME, SETTINGS_NAME) \
JSON_NAME[#NAME] = SETTINGS_NAME.NAME;

#define GENERATE_DESERIALIZE_CYCLE(SOURCE_STR, JSON_NAME, SETTINGS_NAME, UPDATE_QUEUE, FIELDS) \
FIELDS(DECLARE_DESERIALIZE_ITER, SOURCE_STR, JSON_NAME, SETTINGS_NAME, UPDATE_QUEUE)

#define GENERATE_SERIALIZE_CYCLE(JSON_NAME, SETTINGS_NAME, FIELDS) \
FIELDS(DECLARE_SERIALIZE_ITER, JSON_NAME, SETTINGS_NAME)

bool WirelessController::deserializeSettings(String json_str)
{
	JsonDocument json;

	if (deserializeJson(json, json_str))
		return false;

	// Удаление скрытых полей
	json.remove(SETTING_NAMES[SETTING_TYPE::access_key]);
	json.remove(SETTING_NAMES[SETTING_TYPE::wifi_password]);

	GENERATE_DESERIALIZE_CYCLE(json_str, json, settings, settingUpdateQueue, SETTINGS_FIELDS)
	vTaskDelay(100);

	return true;
}

String WirelessController::serializeSettings()
{
	String json_str;
	JsonDocument json;

	GENERATE_SERIALIZE_CYCLE(json, settings, SETTINGS_FIELDS)

	// Удаление скрытых полей
	json.remove(SETTING_NAMES[SETTING_TYPE::access_key]);
	json.remove(SETTING_NAMES[SETTING_TYPE::wifi_password]);
	
	serializeJson(json, json_str);
	return json_str;
}

String WirelessController::serializeTestingResult() {
	String json_str;
	Preferences pref_test;
	JsonDocument json;

	pref_test.begin("testing", false);

	json["buzzer"] = pref_test.getBool("buzzer");
    json["display"] = pref_test.getBool("display");
    json["INA226"] = pref_test.getBool("INA226");
    json["wifi"] = pref_test.getBool("wifi");

	pref_test.end();

	serializeJson(json, json_str);
	return json_str;
}

// void sendMetricsToOpenTSDB()
// {
// 	HTTPClient http;
// 	http.begin("http://<OPENTSDB_SERVER>:<PORT>/api/put");
// 	http.addHeader("Content-Type", "application/json");
// 	String metricsData = raw_data->getJSON();
// 	int httpResponseCode = http.POST(metricsData);
// 	if (httpResponseCode > 0)
// 	{
// 		String response = http.getString();
// 		Serial.println(response);
// 	}
// 	else
// 	{
// 		Serial.printf("Error on sending POST: %d\n", httpResponseCode);
// 	}
// 	http.end();
// }

void WirelessController::wirelessTask(void *pvParameters)
{
	IPAddress HOSTIP;
	AsyncWebServer server(PORT);

	WiFi.begin(settings.wifi_ssid, settings.wifi_password);
	
	while (WiFi.status() != WL_CONNECTED)
		vTaskDelay(500);

	// Получение главной страницы
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { request->redirect(settings.hostname); });
	
	// Получение данных метрик
	server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
		String batteryData = raw_data->getJSON(); request->send(200, "application/json", batteryData); }
	);

	// Получение настроек
	server.on("/settings", HTTP_GET, 
		[](AsyncWebServerRequest *request) {
			if (request->hasHeader("api_key") && request->header("api_key") == settings.access_key) 
			{
				String settingsData = WirelessController::serializeSettings();
				request->send(200, "application/json", settingsData); 
			} 
			else { 
				request->send(403, "application/json", "{\"error\":\"Invalid API key\"}"); 
			} 
		}
	);

	// Запись настроек
	server.on("/settings", HTTP_POST, [](AsyncWebServerRequest *request) {}, 
		nullptr,
		[](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
			if (request->hasHeader("api_key") && request->header("api_key") == settings.access_key)
			{
				String body_str(data, len);

				if (WirelessController::deserializeSettings(body_str))
					request->send(200, "application/json", "{\"message\":\"Settings updated\"}");
				else
					request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
			}
			else
				request->send(403, "application/json", "{\"error\":\"Invalid access key\"}");
		}
	);

	// Запуск тестирования
	server.on("/testing", HTTP_POST,
		[](AsyncWebServerRequest *request) {
			if (request->hasHeader("api_key") && request->header("api_key") == settings.access_key)
			{
				Preferences pref_test;
                pref_test.begin("testing", false);
                pref_test.putBool("test_enabled", true);
                pref_test.end();

				request->send(200, "application/json", "{\"message\":\"Restart ESP32 to start testing...\"}");
			}
			else
				request->send(403, "application/json", "{\"error\":\"Invalid access key\"}");

			ESP.restart();
		}
	);

	// Чтение результатов тестирование
	server.on("/testing", HTTP_GET,
		[](AsyncWebServerRequest *request) {
			if (request->hasHeader("api_key") && request->header("api_key") == settings.access_key)
				request->send(200, "application/json", WirelessController::serializeTestingResult());
			else
				request->send(403, "application/json", "{\"error\":\"Invalid access key\"}");

			ESP.restart();
		}
	);

	server.begin(); // Запускаем сервер

	while(true) {
		//sendMetricsToOpenTSDB();
		vTaskDelay(settings.wireless_delay);
	}
}