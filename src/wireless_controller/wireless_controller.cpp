#include "wireless_controller/wireless_controller.hpp"

#define GENERATE_UNIQUE_BUFFERS(TYPE, F1, F2, ...) \
TYPE buffer_##TYPE;

#define DECLARE_DESERIALIZE_ITER(TYPE, NAME, REBOOT_IS_REQUIRED, INPUT_VALIDATOR, SOURCE_STR, JSON_NAME, SETTINGS_NAME, UPDATE_QUEUE, NEED_REBOOT) \
SettingUpdate update_##NAME; \
if(json[#NAME].is<TYPE>()) { \
	buffer_##TYPE = JSON_NAME[#NAME].as<TYPE>(); \
    if(SETTINGS_INFO[NAME].input_validator == nullptr || SETTINGS_INFO[NAME].input_validator(String(buffer_##TYPE)) == 0) { \
		update_##NAME.key = SETTING_TYPE::NAME; \
		update_##NAME.value = buffer_##TYPE; \
		xQueueSend(UPDATE_QUEUE, &update_##NAME, portMAX_DELAY); \
		if(SETTINGS_INFO[NAME].reboot_is_required) \
        	NEED_REBOOT = true; \
	} \
}

#define DECLARE_SERIALIZE_ITER(TYPE, NAME, REBOOT_IS_REQUIRED, INPUT_VALIDATOR, JSON_NAME, SETTINGS_NAME) \
JSON_NAME[#NAME] = SETTINGS_NAME.NAME;

#define GENERATE_DESERIALIZE_CYCLE(SOURCE_STR, JSON_NAME, SETTINGS_NAME, UPDATE_QUEUE, NEED_REBOOT, FIELDS) \
UNIQUE_SETTINGS_TYPES(GENERATE_UNIQUE_BUFFERS) \
FIELDS(DECLARE_DESERIALIZE_ITER, SOURCE_STR, JSON_NAME, SETTINGS_NAME, UPDATE_QUEUE, NEED_REBOOT) \

#define GENERATE_SERIALIZE_CYCLE(JSON_NAME, SETTINGS_NAME, FIELDS) \
FIELDS(DECLARE_SERIALIZE_ITER, JSON_NAME, SETTINGS_NAME)

unsigned long ota_progress_millis = 0;

void onOTAStart() {
	Serial.println("OTA: Обновление запущено!");
}

void onOTAProgress(size_t current, size_t final) {
	if (millis() - ota_progress_millis > 1000) {
		ota_progress_millis = millis();
		Serial.printf("OTA: Загружено %u байт из конечных %u байт\r\n", current, final);
	}
}

void onOTAEnd(bool success) {
	if (success) {
		Serial.println("OTA: Обновление завершено успешно!");
	} else {
		Serial.println("OTA: Произошли ошибки при обновлении!");
	}
}


bool WirelessController::deserializeSettings(String json_str, bool &needReboot)
{
	JsonDocument json;

	if (deserializeJson(json, json_str))
		return false;

	// Удаление скрытых полей
	json.remove(SETTINGS_INFO[SETTING_TYPE::access_key].name);
	json.remove(SETTINGS_INFO[SETTING_TYPE::influxdb_token].name);

	GENERATE_DESERIALIZE_CYCLE(json_str, json, settings, settingUpdateQueue, needReboot, SETTINGS_FIELDS)
	vTaskDelay(100);

	return true;
}

String WirelessController::serializeSettings()
{
	String json_str;
	JsonDocument json;

	GENERATE_SERIALIZE_CYCLE(json, settings, SETTINGS_FIELDS)

	// Удаление скрытых полей
	json.remove(SETTINGS_INFO[SETTING_TYPE::access_key].name);
	json.remove(SETTINGS_INFO[SETTING_TYPE::influxdb_token].name);
	
	serializeJson(json, json_str);
	return json_str;
}

String WirelessController::serializeTestingResult() {
	String json_str;
	JsonDocument json;
	IntegrationTestResult result = UnitedControl::readTestResults();

	json["buzzer"] = result.buzzerTest;
    json["display"] = result.displayTest;
    json["INA226"] = result.ina226Test;
    json["wifi"] = result.wifiTest;
	json["database"] = result.databaseTest;

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
				bool needReboot;

				if (WirelessController::deserializeSettings(body_str, needReboot)) {
					if(!needReboot)
						request->send(200, "application/json", "{\"message\":\"Settings updated\"}");
					else {
						request->send(200, "application/json", "{\"message\":\"Restart to update settings...\"}");
						UnitedControl::restartSystem();
					}
				}
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
				UnitedControl::startTest(false);

				request->send(200, "application/json", "{\"message\":\"Restart ESP32 to start testing...\"}");
			}
			else
				request->send(403, "application/json", "{\"error\":\"Invalid access key\"}");

			UnitedControl::restartSystem();
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

			UnitedControl::restartSystem();
		}
	);


	// OTA
	ElegantOTA.begin(&server);

	String user = String("battery_") + settings.battery_id;
	String password = settings.access_key;
	ElegantOTA.setAuth(user.c_str(), password.c_str());
	ElegantOTA.setAutoReboot(true);
	ElegantOTA.onStart(onOTAStart);
	ElegantOTA.onProgress(onOTAProgress);
	ElegantOTA.onEnd(onOTAEnd);
	//


	server.begin(); // Запускаем сервер


	// Настраиваем работу с СУБД
	Point data_point("battery");
	
	currentTimeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov", "0.ru.pool.ntp.org");
	client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::S).batchSize(10).bufferSize(30).flushInterval(30).maxRetryInterval(60)); // Конфигурация
	
	data_point.addTag("device", device_hostname);
	while(true) {
		ElegantOTA.loop();

		if (WiFi.status() == WL_CONNECTED) {
			if (client.validateConnection()) {
				data_point.clearFields();

				data_point.addField("voltage", raw_data->voltage, 2);
				data_point.addField("current", raw_data->current, 2);
				data_point.addField("power", raw_data->power, 2);
				data_point.addField("capacity", raw_data->capacity, 2);
				data_point.setTime(raw_data->timestamp);

				client.writePoint(data_point);
			}
		}

		vTaskDelay(settings.wireless_delay);
	}
}