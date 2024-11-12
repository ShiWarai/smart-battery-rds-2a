#include "display_controller/display_controller.hpp"

void DisplayController::displayTask(void *pvParameters) {

	SemaphoreHandle_t wireMutex = static_cast<SemaphoreHandle_t>(pvParameters);

	U8G2_SSD1306_64X32_1F_F_HW_I2C oled = U8G2_SSD1306_64X32_1F_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

	bool display_enabled = false;
    const int display_frequency = 200;

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
			if(display_button.hold()){
				invertMode();
			}else if(display_button.click()) // Сейчас горит всегда
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

				for(int i = 0; i < (settings.display_time/display_frequency); i++)
				{
					if (xSemaphoreTake(wireMutex, portMAX_DELAY) == pdTRUE)
					{
						DisplayController::printStatus(&oled, *raw_data);
						
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
			Serial.println("A");
			if(display_button.hold()){invertMode();}
			if (xSemaphoreTake(wireMutex, portMAX_DELAY) == pdTRUE)
			{
				DisplayController::printStatus(&oled, *raw_data);
				xSemaphoreGive(wireMutex);
				vTaskDelay(display_frequency);
			}
		}
	}
}

void DisplayController::invertMode() {
	Serial.print("!mode");
	SettingUpdate update;
	update.value = (uint32_t)!settings.mode;
	update.key = SETTING_TYPE::mode;
	Serial.println(!settings.mode);
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