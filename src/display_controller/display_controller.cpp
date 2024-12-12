#include "display_controller/display_controller.hpp"

void DisplayController::displayTask(void *pvParameters) {

	SemaphoreHandle_t wireMutex = static_cast<SemaphoreHandle_t>(pvParameters);

	U8G2_SSD1306_64X32_1F_F_HW_I2C oled = U8G2_SSD1306_64X32_1F_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

	bool display_enabled = false;
	unsigned long current_time;
	unsigned long display_shutdown_timer;
    const int display_frequency = 100;
	SCREEN_MODE screen_mode = settings.mode == BATTERY_MOD::FULL ? SCREEN_MODE::MAIN : SCREEN_MODE::NONE;

	Button display_button(BUTTONS_PIN, INPUT);
	pinMode(OLED_PWR_PIN, OUTPUT);
	pinMode(BUZZER_PIN, OUTPUT);

	digitalWrite(OLED_PWR_PIN, HIGH);
	oled.begin();
	digitalWrite(OLED_PWR_PIN, LOW);

	unsigned long last_time = millis();
	while(true) {
		display_button.tick();
		current_time = millis();

		if(settings.mode == BATTERY_MOD::POWERSAVE) {
			display_shutdown_timer += (current_time - last_time);
			
			if(display_shutdown_timer >= settings.display_time)
				screen_mode = SCREEN_MODE::NONE;
		}

		last_time = current_time;

		if(display_button.holdFor(CHANGE_MODE_HOLD_TIME))
			invertMode();
		
		if(display_button.hold(3)) {
			nvs_flash_erase();
			nvs_flash_init();
			ESP.restart();
		}
		
		if(display_button.click()) {
			screen_mode = (SCREEN_MODE) ((screen_mode + 1) % SCREEN_MODE::COUNT);

			switch(settings.mode) {
				case BATTERY_MOD::POWERSAVE:
					if(screen_mode > SCREEN_MODE::NONE && !display_enabled) {
						if (xSemaphoreTake(wireMutex, portMAX_DELAY) == pdTRUE) // Забираем управление I2C и делаем перезапуск датчика
						{
							DisplayController::turnOnDisplay(&oled);

							xSemaphoreGive(wireMutex);
							vTaskDelay(1);
							
							display_enabled = true;
						}
					}

					display_shutdown_timer = 0;
					break;
				case BATTERY_MOD::FULL:
					if(screen_mode == SCREEN_MODE::NONE)
						screen_mode = SCREEN_MODE::MAIN;
					break;
			}
		}

		if(screen_mode != SCREEN_MODE::NONE) {
			if (xSemaphoreTake(wireMutex, portMAX_DELAY) == pdTRUE) {
				switch(screen_mode) {
					case SCREEN_MODE::MAIN:
						DisplayController::printStatus(&oled, *raw_data);
						break;
					case SCREEN_MODE::WIFI: 
						DisplayController::printSecondMenu(&oled, *raw_data);
						break;
					case SCREEN_MODE::POWER_CHART: 
						DisplayController::printHistoryMenu(&oled, *raw_data, screen_mode, 1);
						break;
					case SCREEN_MODE::VOLTAGE_CHART: 
						DisplayController::printHistoryMenu(&oled, *raw_data, screen_mode, 1);
						break;
					default:
						break;
				}

				xSemaphoreGive(wireMutex);
				vTaskDelay(display_frequency);
			}
		} else {
			if (display_enabled) {
				DisplayController::turnOffDisplay(&oled);
				display_enabled = false;
			}
			
			vTaskDelay(100);
		}
	}
}

void DisplayController::invertMode() {
	SettingUpdate update;

	update.value = (uint32_t)!settings.mode;
	update.key = SETTING_TYPE::mode;

	xQueueSend(settingUpdateQueue, &update, portMAX_DELAY);

	vTaskDelay(1000);

	ESP.restart();
}

void DisplayController::turnOnDisplay(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled) {
	digitalWrite(OLED_PWR_PIN, HIGH);
	Wire.end();
	oled->begin();
}

void DisplayController::turnOffDisplay(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled) {
	oled->clearDisplay();
	digitalWrite(OLED_PWR_PIN, LOW);	
}

void DisplayController::printStatus(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled, INA226Data data, bool changeContrast, byte contrast)
{
	// clear frame buffer and set display brightness if needed
	oled->clearBuffer();
	if (changeContrast) {
		oled->setContrast(contrast);
	}

	// obtain voltage from power monitor and prepare values
	uint32_t id = *data.id;
	double voltage = data.voltage;
	double percentage = std::max(std::min(data.capacity, 100.0f), 0.0f); // Удаляем потенциальное неподходящее значение
	double power = data.power;
	
	// create a string with formatted percentage value
	String buffer;

	// рисуем номер аккумулятора
	buffer = String("ID:") + String(id);
	buffer.trim();
	oled->setFont(u8g2_font_4x6_mr);
	oled->drawStr(0, 6, buffer.c_str());

	// display voltage based battery charge percentage, accounting for the number of digits
	buffer = String(percentage,0) + String("%");
	buffer.trim();
	oled->setFont(u8g2_font_spleen12x24_mu); // set big font
	if(buffer.length() == 2) {
		oled->drawStr(30, 16, buffer.c_str());
	} else if(buffer.length() == 3) {
		oled->drawStr(30, 16, buffer.c_str());
	} else {
		oled->drawStr(22, 16, buffer.c_str());
	}

	// prepare a string with formatted voltage value
	buffer = String(voltage,2);
	
	// display true battery voltage and fake power value on the bottom
	oled->setFont(u8g2_font_spleen6x12_mr); // set smaller font
	buffer = String(voltage,2);
	buffer.trim();
	oled->drawStr(0, 31, buffer.c_str());
	oled->drawStr(24, 31, "v");
	buffer = String(power,2);
	buffer.trim();
	oled->drawStr(33, 31, buffer.c_str());
	oled->drawStr(57, 31, "w");
	//if(WirelessController::wificonnect){oled->drawStr(0, 15, "wifi");}
	
	// send frame buffer to the display
	oled->sendBuffer();
}

void DisplayController::printSecondMenu(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled, INA226Data data, bool changeContrast, byte contrast) // routine for printing simple interface on an OLED display
{
	// clear frame buffer and set display brightness if needed
	oled->clearBuffer();
	if (changeContrast) {
		oled->setContrast(contrast);
	}

	// obtain voltage from power monitor and prepare values
	uint32_t id = *data.id;
	String wifissid = settings.wifi_ssid;
	//WiFi.status()

	// create a string with formatted percentage value
	String buffer;
	
	// рисуем номер аккумулятора
	buffer = String(WiFi.RSSI())+String("dBm");
	buffer.trim();
	oled->setFont(u8g2_font_4x6_mr);
	oled->drawStr(0, 6, buffer.c_str());
	

	const uint8_t wifiBitmap8x6[] = {
		0x7E, // 01111110
		0x81, // 10000001
		0x3C, // 00111100
		0x42, // 01000010
		0x18, // 00011000
		0x18  // 00011000
	};
	
	// рисуем статус wifi
	oled->setFont(u8g2_font_4x6_mr);
	if(WiFi.status()==WL_CONNECTED){oled->drawBitmap(56, 0, 1,6,wifiBitmap8x6);
	}else{oled->drawBox(59,4,2,2);}

	// рисуем wifi ssid
	buffer = String(wifissid);
	buffer.trim();
	oled->setFont(u8g2_font_4x6_mr); // set big font
	oled->drawStr(0, 17, "SSID:");
	oled->drawStr(0, 28, buffer.c_str());

	oled->sendBuffer();
}

void DisplayController::printHistoryMenu(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled, INA226Data data, SCREEN_MODE mode, bool scale, bool changeContrast, byte contrast) // routine for printing simple interface on an OLED display
{
	oled->clearBuffer();
	if (changeContrast) {
		oled->setContrast(contrast);
	}

	// obtain voltage from power monitor and prepare values
	uint32_t id = *data.id;
	
	// create a string with formatted percentage value
	String buffer;

	// рисуем номер аккумулятора
	buffer = String("ID:") + String(id);
	buffer.trim();
	oled->setFont(u8g2_font_4x6_mr);
	oled->drawStr(0, 6, buffer.c_str());

	//рисуем историю
	oled->setFont(u8g2_font_4x6_mr);

	std::pair<std::deque<float>::iterator, std::deque<float>::iterator> minmax;
	switch(mode) {
		case SCREEN_MODE::POWER_CHART:
			oled->drawStr(25, 5, "Power");

			// берём крайние значения
			minmax = std::minmax_element(std::begin(raw_data->powerBuffer), std::end(raw_data->powerBuffer));
			
			// рисуем график
			for(int i=0; i < METRICS_BUFFER_SIZE; i++){
				int hhist = (int)FLOAT_MAP(raw_data->powerBuffer[i], *minmax.first, *minmax.second, 1.0, 25.0);
				oled->drawBox(62-((2-scale)*i), 32-hhist, 1, hhist);
			}
			break;
		case SCREEN_MODE::VOLTAGE_CHART:
			// берём крайние значения
			minmax = std::minmax_element(std::begin(raw_data->voltageBuffer), std::end(raw_data->voltageBuffer));
			buffer = String("V(")+String(*minmax.first, 1)+String("-")+String(*minmax.second, 1)+String(")");
			oled->drawStr(22, 5, buffer.c_str());
			// рисуем график
			for(int i=0; i < METRICS_BUFFER_SIZE; i++){
				int hhist = (int)FLOAT_MAP(raw_data->voltageBuffer[i], *minmax.first, *minmax.second, 1.0, 25.0);
				oled->drawBox(62-((2-scale)*i), 32-hhist, 1, hhist);
			}
			break;
	}

	// send frame buffer to the display
	oled->sendBuffer();
}