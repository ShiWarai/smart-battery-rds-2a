#include "wireless_controller/wireless_controller.hpp"

bool deserializeSettings(String json)
{
	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, json);

	if (error)
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

	doc["sensor_delay"] = settings.sensor_delay;
	doc["usb_delay"] = settings.usb_delay;
	doc["wireless_delay"] = settings.wireless_delay;
	doc["display_time"] = settings.display_time;
	doc["hostname"] = settings.hostname;
	doc["wifi_ssid"] = settings.wifi_ssid;
	doc["wifi_password"] = settings.wifi_password;
	doc["mode"] = settings.mode;
	doc["battery_id"] = settings.battery_id;
	
	serializeJson(doc, json);
	return json;
}

void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  if(!index){
    Serial.printf("BodyStart: %u B\n", total);
  }
  for(size_t i=0; i<len; i++){
    Serial.write(data[i]);
  }
  if(index + len == total){
    Serial.printf("BodyEnd: %u B\n", total);
  }
}

void WirelessController::wirelessTask(void *pvParameters)
{
	IPAddress HOSTIP;
	AsyncWebServer server(80);

	WiFi.begin(settings.wifi_ssid, settings.wifi_password);
	
	while (WiFi.status() != WL_CONNECTED)
		vTaskDelay(500);

	while(mdns_init()!= ESP_OK) vTaskDelay(500); // ожидание запуска mDNS

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(200, "text/plain", "Hello, World!"); });
	
	server.on("/battery", HTTP_GET, [](AsyncWebServerRequest *request)
	{ String batteryData = raw_data->getJSON(); request->send(200, "application/json", batteryData); });

	server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		if (request->hasHeader("api_key") && request->header("api_key") == settings.access_key) 
		{
			String settingsData = getSettingsJSON();
			request->send(200, "application/json", settingsData); 
		} 
		else { 
			request->send(403, "application/json", "{\"error\":\"Invalid API key\"}"); 
		} 
	});

	server.on("/settings", HTTP_POST, [](AsyncWebServerRequest *request)
	{
		if (request->hasHeader("api_key") && request->header("api_key") == settings.access_key)
		{
			if (deserializeSettings(request->getParam("body")->value()))
			{
				request->send(200, "application/json", "{\"message\":\"Settings updated\"}");
			}
			else
			{
				request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
			}
		}
		else
		{
			request->send(403, "application/json", "{\"error\":\"Invalid access key\"}");
		} 
	}, nullptr, handleBody);

	server.begin(); // Запускаем сервер

	while(true) {
		vTaskDelay(settings.wireless_delay);
	}
}