#include "display_controller/display_controller.hpp"

void DisplayController::displayTask(void *pvParameters) {

	SemaphoreHandle_t wireMutex = static_cast<SemaphoreHandle_t>(pvParameters);

	U8G2_SSD1306_64X32_1F_F_HW_I2C oled = U8G2_SSD1306_64X32_1F_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

	bool display_enabled = false;
    const int display_frequency = 200;
	uint16_t screen;
	DisplayController::lastUpdtHistTime = millis();

	Button display_button(BUTTONS_PIN, INPUT);
	pinMode(OLED_PWR_PIN, OUTPUT);
	pinMode(BUZZER_PIN, OUTPUT);

	digitalWrite(OLED_PWR_PIN, HIGH);
	oled.begin();
	digitalWrite(OLED_PWR_PIN, LOW);

	switch(settings.mode)
	{
	case BATTERY_MODS::POWERSAVE:
		while(true) {
			display_button.tick();
			updatingHistory(*raw_data);

			if(display_button.holdFor(CHANGE_MODE_HOLD_TIME))
				invertMode();

			else if(display_button.click()) // Сейчас горит всегда
			{
				if(!display_enabled) {
					if (xSemaphoreTake(wireMutex, portMAX_DELAY) == pdTRUE) // Забираем управление I2C и делаем перезапуск датчика
					{
						DisplayController::turnOnDisplay(&oled);

						xSemaphoreGive(wireMutex);
						vTaskDelay(1);
						
						display_enabled = true;
					}
				}

				screen=0;

				for(int i = 0; i < (settings.display_time/display_frequency); i++)
				{
					updatingHistory(*raw_data);
					display_button.tick();
					if(display_button.click()){screen++;i=0;}
					if (xSemaphoreTake(wireMutex, portMAX_DELAY) == pdTRUE)
					{
						screenSwitch(&oled, screen);
						
						xSemaphoreGive(wireMutex);
						vTaskDelay(display_frequency);
					}
				}
			} else if (display_enabled) {
				DisplayController::turnOffDisplay(&oled);
				display_enabled = false;
			}
		}
	case BATTERY_MODS::FULL:
		while(true) {
			display_button.tick();
			updatingHistory(*raw_data);

			if(display_button.holdFor(CHANGE_MODE_HOLD_TIME))
				invertMode();

			if(display_button.click()){screen++;}

			if (xSemaphoreTake(wireMutex, portMAX_DELAY) == pdTRUE)
			{
				screenSwitch(&oled, screen);
				xSemaphoreGive(wireMutex);
				vTaskDelay(display_frequency);
			}
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

void DisplayController::screenSwitch(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled, uint16_t screen) {
	switch(screen%4) {
		case 0: DisplayController::printStatus(oled, *raw_data); break;
		case 1: DisplayController::printSecondMenu(oled, *raw_data); break;
		case 2: DisplayController::printHistoryMenu(oled, *raw_data, 0); break;
		case 3: DisplayController::printHistoryMenu(oled, *raw_data, 1); break;
	}
}

void DisplayController::printStatus(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled, INA226Data data, bool changeContrast, byte contrast) // routine for printing simple interface on an OLED display
{
	// clear frame buffer and set display brightness if needed
	oled->clearBuffer();
	if (changeContrast) {
		oled->setContrast(contrast);
	}

	// obtain voltage from power monitor and prepare values
	uint32_t id = *data.id;
	double voltage = data.voltage;
	double percentage = data.capacity;
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
	buffer = String("ID:") + String(id);
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

	oled->drawBitmap(20, 0, 1,6,wifiBitmap8x6);
	// рисуем статус wifi
	if(WiFi.status()==WL_CONNECTED){
	oled->setFont(u8g2_font_4x6_mr); // set big font
	oled->drawStr(29, 6, "connected");}

	// рисуем wifi ssid
	buffer = String(wifissid);
	buffer.trim();
	oled->setFont(u8g2_font_spleen6x12_mr); // set big font
	oled->drawStr(0, 17, buffer.c_str());
	oled->drawStr(-64, 28, buffer.c_str());

	oled->sendBuffer();
}

void DisplayController::printHistoryMenu(U8G2_SSD1306_64X32_1F_F_HW_I2C *oled, INA226Data data, bool scale, bool changeContrast, byte contrast) // routine for printing simple interface on an OLED display
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
	oled->drawStr(25, 5, "history");

	// берём крайние значения
    int minHistry = DisplayController::powerHistory[0];
    int maxHistry = DisplayController::powerHistory[0];
    for (int i = 1; i < (scale+1)*30; i++) {
        if (DisplayController::powerHistory[i] < minHistry)
            minHistry = DisplayController::powerHistory[i];
        if (DisplayController::powerHistory[i] > maxHistry)
            maxHistry = DisplayController::powerHistory[i];
    }
	
	// рисуем график
	for(int i=0;i<(scale+1)*30;i++){
		int hhist=map(long(DisplayController::powerHistory[i]),long(minHistry),long(maxHistry),long(1),long(25));
		oled->drawBox(62-((2-scale)*i), 32-hhist, 1, hhist);
	}

	// send frame buffer to the display
	oled->sendBuffer();
}

void DisplayController::updatingHistory(INA226Data data){
	if(millis()-DisplayController::lastUpdtHistTime>=1000){
		for(int i=60-1;i>0;i--){//сдвиг
			DisplayController::powerHistory[i]=DisplayController::powerHistory[i-1];
		}
		DisplayController::powerHistory[0]=data.power;//запись нового значения
		DisplayController::lastUpdtHistTime=millis();
	}
}