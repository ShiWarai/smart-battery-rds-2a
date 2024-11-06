#include "wireless_controller/wireless_controller.hpp"

bool deserializeSettings(String json)
{
	JsonDocument doc;

	if (deserializeJson(doc, json))
	{
		return false;
	}

	settings.sensor_delay = doc["sensor_delay"];
	settings.usb_delay = doc["usb_delay"];
	settings.wireless_delay = doc["wireless_delay"];
	settings.display_time = doc["display_time"];
	settings.hostname = doc["hostname"].as<String>();
	settings.wifi_ssid = doc["wifi_ssid"].as<String>();
	settings.wifi_password = doc["wifi_password"].as<String>();
	settings.mode = doc["mode"];
	settings.battery_id = doc["battery_id"];

	return true;
}

String getSettingsJSON()
{
	String json;
	JsonDocument doc;

	doc["battery_id"] = settings.battery_id;
	doc["sensor_delay"] = settings.sensor_delay;
	doc["usb_delay"] = settings.usb_delay;
	doc["wireless_delay"] = settings.wireless_delay;
	doc["display_time"] = settings.display_time;
	doc["hostname"] = settings.hostname;
	doc["wifi_ssid"] = settings.wifi_ssid;
	doc["wifi_password"] = settings.wifi_password;
	doc["mode"] = settings.mode;
	
	serializeJson(doc, json);
	return json;
}

bool setSetting(String name, String value)
{
	if (name == "sensor_delay")
		settings.sensor_delay = value.toInt();
	else if (name == "usb_delay")
		settings.usb_delay = value.toInt();
	else if (name == "wireless_delay")
		settings.wireless_delay = value.toInt();
	else if (name == "display_time")
		settings.display_time = value.toInt();
	else if (name == "hostname")
		settings.hostname = value;
	else if (name == "wifi_ssid")
		settings.wifi_ssid = value;
	else if (name == "wifi_password")
		settings.wifi_password = value;
	else if (name == "mode")
		settings.mode = value.toInt();
	else if (name == "battery_id")
		settings.battery_id = value.toInt();
	else
		return false;


	Serial.println(value);
	Serial.println(settings.battery_id);

	return true;
}

String getSetting(String name)
{
	if (name == "sensor_delay")
		return String(settings.sensor_delay);
	else if (name == "usb_delay")
		return String(settings.usb_delay);
	else if (name == "wireless_delay")
		return String(settings.wireless_delay);
	else if (name == "display_time")
		return String(settings.display_time);
	else if (name == "hostname")
		return settings.hostname;
	else if (name == "wifi_ssid")
		return settings.wifi_ssid;
	else if (name == "wifi_password")
		return settings.wifi_password;
	else if (name == "mode")
		return String(settings.mode);
	else if (name == "battery_id")
		return String(settings.battery_id);
	else
		return "";
}

void WirelessController::wirelessTask(void *pvParameters)
{
	IPAddress HOSTIP;
	AsyncWebServer server(PORT);

	WiFi.begin(settings.wifi_ssid, settings.wifi_password);
	
	while (WiFi.status() != WL_CONNECTED)
		vTaskDelay(500);

	while(mdns_init()!= ESP_OK) vTaskDelay(500); // ожидание запуска mDNS

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
				String settingsData = getSettingsJSON();
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

				if (deserializeSettings(body_str))
					request->send(200, "application/json", "{\"message\":\"Settings updated\"}");
				else
					request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
			}
			else
				request->send(403, "application/json", "{\"error\":\"Invalid access key\"}");
		}
	);

    server.on("^\\/setting\\/([a-zA-Z0-9_]+)$", HTTP_GET, 
		[](AsyncWebServerRequest *request) {
        if (request->hasHeader("api_key") && request->header("api_key") == settings.access_key) {
            String settingName = request->pathArg(0);
            String response;

            if (settingName == "sensor_delay") response = String(settings.sensor_delay);
            else if (settingName == "usb_delay") response = String(settings.usb_delay);
            else if (settingName == "wireless_delay") response = String(settings.wireless_delay);
            else if (settingName == "display_time") response = String(settings.display_time);
            else if (settingName == "hostname") response = settings.hostname;
            else if (settingName == "wifi_ssid") response = settings.wifi_ssid;
            else if (settingName == "wifi_password") response = settings.wifi_password;
            else if (settingName == "mode") response = String(settings.mode);
            else if (settingName == "battery_id") response = String(settings.battery_id);
            else {
                request->send(404, "application/json", "{\"error\":\"Setting not found\"}");
                return;
            }
            request->send(200, "application/json", "{\"" + settingName + "\":\"" + response + "\"}");
        } else
            request->send(403, "application/json", "{\"error\":\"Invalid API key\"}");
    });

	server.on("^/setting/([a-zA-Z0-9_]+)$", HTTP_PUT, 
		[](AsyncWebServerRequest *request) {}, 
		NULL, 
		[](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
			if (request->hasHeader("api_key") && request->header("api_key") == settings.access_key) 
			{
				String name = request->pathArg(0);
				String value = String(data, len);

				if (setSetting(name, value))
					request->send(200, "application/json", "{\"message\":\"Setting updated\"}");
				else
					request->send(404, "application/json", "{\"error\":\"Setting not found\"}");
			} 
			else
				request->send(403, "application/json", "{\"error\":\"Invalid API key\"}");
		}
	);

	server.begin(); // Запускаем сервер

	while(true) {
		vTaskDelay(settings.wireless_delay);
	}
}