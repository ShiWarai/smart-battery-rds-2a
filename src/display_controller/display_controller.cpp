#include "display_controller/display_controller.hpp"

void DisplayController::displayTask(void *pvParameters) {

	SemaphoreHandle_t wireMutex = static_cast<SemaphoreHandle_t>(pvParameters);

	U8G2_SSD1306_64X32_1F_F_HW_I2C oled = U8G2_SSD1306_64X32_1F_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

	bool display_enabled = false;
    const int display_frequency = 200;

	pinMode(OLED_PWR_PIN, OUTPUT);
	pinMode(BUTTONS_PIN, INPUT);
	pinMode(BUZZER_PIN, OUTPUT);

	digitalWrite(OLED_PWR_PIN, HIGH);
	oled.begin();
	digitalWrite(OLED_PWR_PIN, LOW);

	switch(settings.mode)
	{
	case BATTERY_MODS::POWERSAVE:
		while(true) {
			if(!digitalRead(BUTTONS_PIN)) // Сейчас горит всегда
			{
				if(!display_enabled) {
					digitalWrite(OLED_PWR_PIN, HIGH);
					if (xSemaphoreTake(wireMutex, portMAX_DELAY) == pdTRUE) // Забираем управление I2C и делаем перезапуск датчика
					{
						Wire.end();
						oled.begin();
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
				oled.clearDisplay();
				digitalWrite(OLED_PWR_PIN, LOW);
				display_enabled = false;
			}

			vTaskDelay(100);
		}
	case BATTERY_MODS::FULL:
		while(true) {
			if (xSemaphoreTake(wireMutex, portMAX_DELAY) == pdTRUE)
			{
				DisplayController::printStatus(&oled, *raw_data);
				
				xSemaphoreGive(wireMutex);
				vTaskDelay(display_frequency);
			}
		}
	}
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
	double voltage = data.voltage;
	double p = data.capacity;
	String power = String(data.power,2);
	power.trim();
	
	// create a string with formatted percentage value
	String s;
	s = String(p,0);
	s.trim();

	// display voltage based battery charge percentage, accounting for the number of digits
	oled->setFont(u8g2_font_spleen16x32_mu); // set big font
	if(s.length() == 1) {
		oled->drawStr(34, 20, s.c_str());
	} else if(s.length() == 2) {
		oled->drawStr(18, 20, s.c_str());
	} else {
		oled->drawStr(2, 20, s.c_str());
	}
	oled->drawStr(50, 20, "%");

	// prepare a string with formatted voltage value
	s = String(voltage,2);
	
	// display true battery voltage and fake power value on the bottom
	oled->setFont(u8g2_font_spleen6x12_mr); // set smaller font
	oled->drawStr(0, 31, s.c_str());
	oled->drawStr(24, 31, "v");
	oled->drawStr(33, 31, power.c_str());
	oled->drawStr(57, 31, "w");
	
	// send frame buffer to the display
	oled->sendBuffer();
}