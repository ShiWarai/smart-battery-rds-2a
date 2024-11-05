#include "wireless_controller/wireless_controller.hpp"

void WirelessController::wirelessTask(void *pvParameters) {	
	IPAddress HOSTIP;
	WiFiClient client;

	WiFi.begin(settings.wifi_ssid, settings.wifi_password);
	
	while (WiFi.status() != WL_CONNECTED) {
		// wificonnect=0;
		vTaskDelay(500);}
	// wificonnect=1;//подключение установлено
	// Serial.print("WiFi connected with IP: ");
	// Serial.println(WiFi.localIP());

	while(mdns_init()!= ESP_OK) vTaskDelay(500); // ожидание запуска mDNS

	while(true) {
		// поиск IP-адреса hostname
		#ifdef HOSTNAME
		if (HOSTIP.toString() == "0.0.0.0") {
			HOSTIP = MDNS.queryHost(settings.hostname);
		} else
		#endif
		if(!client.connected()) // если потеряли связь с клиентом, то устонавливаем её заново
		{
			// clientconnect=0;
			#ifdef HOSTNAME
				client.connect(HOSTIP, PORT);
			#else
				client.connect("host.wokwi.internal", PORT);
			#endif
		} else {
			// clientconnect=1;
			client.print(raw_data->getJSON());
		}

		vTaskDelay(settings.wireless_delay);
	}
}