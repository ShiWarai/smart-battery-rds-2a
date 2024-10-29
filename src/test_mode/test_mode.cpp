#include "test_mode\test_mode.hpp"

void TestMode::test(){
    // дисплей
    U8G2_SSD1306_64X32_1F_F_HW_I2C oled = U8G2_SSD1306_64X32_1F_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);
    
    pinMode(OLED_PWR_PIN, OUTPUT);
	pinMode(BUTTONS_PIN, INPUT);
	pinMode(BUZZER_PIN, OUTPUT);

    digitalWrite(OLED_PWR_PIN, HIGH);
    
	vTaskDelay(100);
	oled.begin();
    vTaskDelay(100);

    oled.clearBuffer();
    oled.setFont(u8g2_font_spleen16x32_mu);
    oled.drawStr(10, 10, "TEST");
    oled.sendBuffer();

	vTaskDelay(2000);
    digitalWrite(OLED_PWR_PIN, LOW);
    // INA
    INA226 ina226(0x40);
    Wire.begin();

    if (ina226.begin()) {

    }
    //
    
}