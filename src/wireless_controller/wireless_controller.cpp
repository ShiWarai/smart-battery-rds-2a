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
	//json.remove(SETTING_NAMES[SETTING_TYPE::wifi_password]);
	json.remove(SETTING_NAMES[SETTING_TYPE::influxdb_token]);

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
	//json.remove(SETTING_NAMES[SETTING_TYPE::wifi_password]);
	json.remove(SETTING_NAMES[SETTING_TYPE::influxdb_token]);
	
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
	json["database"] = pref_test.getBool("database");

	pref_test.end();

	serializeJson(json, json_str);
	return json_str;
}

void currentTimeSync(const char *tzInfo, const char* ntpServer1, const char* ntpServer2 = nullptr, const char* ntpServer3 = nullptr) {
	configTzTime(tzInfo, ntpServer1, ntpServer2, ntpServer3);

	while (time(nullptr) < 1000000000l)
		delay(500);
}

void WirelessController::wirelessTask(void *pvParameters)
{
	String device_hostname = String("battery_") + String(settings.battery_id);

	AsyncWebServer server(PORT); // Веб-сервер
	InfluxDBClient client(settings.influxdb_url, settings.influxdb_org, settings.influxdb_bucket, settings.influxdb_token); // Сервис для отправки в InfluxDB

	WiFi.setHostname(device_hostname.c_str()); 
	WiFi.begin(settings.wifi_ssid, settings.wifi_password);

	while (WiFi.status() != WL_CONNECTED)
		vTaskDelay(500);

	while (!MDNS.begin(device_hostname.c_str()))
		vTaskDelay(500);

	MDNS.addService("http", "tcp", 80);
	

	// Получение главной страницы
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { request->redirect(settings.hostname); });

	// Тестовое получение данных для пинга по HTTP
	server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "application/json", "{\"message\":\"OK\"}"); }
	);
	
	// Получение данных метрик
	server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "application/json", raw_data->getJSON()); }
	);

	// Получение настроек
	server.on("/settings", HTTP_GET, 
		[](AsyncWebServerRequest *request) {
			if (request->hasHeader("api_key") && request->header("api_key") == settings.access_key) 
			{
				request->send(200, "application/json", WirelessController::serializeSettings()); 
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
		}
	);

	// Рестарт
	server.on("/restart", HTTP_POST,
		[](AsyncWebServerRequest *request) {
			if (request->hasHeader("api_key") && request->header("api_key") == settings.access_key)
				request->send(200, "application/json", "{\"message\":\"Restart ESP32...\"}");
			else
				request->send(403, "application/json", "{\"error\":\"Invalid access key\"}");

			vTaskDelay(1000);

			ESP.restart();
		}
	);

	server.begin(); // Запускаем сервер

	// Настраиваем работу с СУБД
	Point data_point("battery");
	
	currentTimeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov", "0.ru.pool.ntp.org");
	client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::S).batchSize(10).bufferSize(30).flushInterval(30).maxRetryInterval(60)); // Конфигурация
	
	data_point.addTag("device", device_hostname);
	while(true) {
		if (WiFi.status() == WL_CONNECTED) {
			if (client.validateConnection()) {
				data_point.clearFields();

				data_point.addField("voltage", raw_data->voltage);
				data_point.addField("current", raw_data->current);
				data_point.addField("power", raw_data->power);
				data_point.addField("capacity", raw_data->capacity);
				data_point.setTime(time(nullptr));

				client.writePoint(data_point);
			}
		}

		vTaskDelay(settings.wireless_delay);
	}
}