#include "wireless_controller/wireless_controller.hpp"

void WirelessController::wirelessTask(void *pvParameters) {	
	IPAddress HOSTIP;
	AsyncWebServer server(80);

	WiFi.begin(settings.wifi_ssid, settings.wifi_password);
	
	while (WiFi.status() != WL_CONNECTED)
		vTaskDelay(500);

	while(mdns_init()!= ESP_OK) vTaskDelay(500); // ожидание запуска mDNS

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(200, "text/plain", "Hello, World!"); });
	
	server.on("/battery", HTTP_GET, [](AsyncWebServerRequest *request)
			{ String batteryData = raw_data->getJSON(); request->send(200, "application/json", batteryData); });
		
	server.on("/settings", HTTP_POST, [](AsyncWebServerRequest *request)
				{
		if (request->hasHeader("access_key") && request->header("access_key") == settings.access_key)
		{
			String body = request->getParam("body")->value();
			StaticJsonDocument<256> doc;
			DeserializationError error = deserializeJson(doc, body);
			if (error)
			{
				request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
				return;
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
			request->send(200, "application/json", "{\"message\":\"Settings updated\"}");
		}
		else
		{
			request->send(403, "application/json", "{\"error\":\"Invalid access key\"}");
		} });
	server.begin();

	while(true) {
		vTaskDelay(settings.wireless_delay);
	}
}