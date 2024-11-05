#include "test_mode\test_mode.hpp"

void TestMode::test(){
    // ------BUZZER------
    digitalWrite(BUZZER_PIN, HIGH);
    delay(2000);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Пищалка прозвучала");

    // ---------дисплей---------
    pinMode(OLED_PWR_PIN, OUTPUT); 
    pinMode(BUTTONS_PIN, INPUT); 
    pinMode(BUZZER_PIN, OUTPUT); 
    digitalWrite(OLED_PWR_PIN, HIGH); // Включение питания дисплея 
    delay(100); // Задержка для стабильной инициализации 
    // Инициализация дисплея 
    U8G2_SSD1306_64X32_1F_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);
    oled.begin(); delay(100); 
    // Очистка буфера дисплея и вывод текста 
    oled.clearBuffer(); 
    oled.setFont(u8g2_font_spleen16x32_mu); 
    oled.drawStr(5, 20, "TEST"); 
    oled.sendBuffer(); 
    delay(2000); // Задержка на 2 секунды для отображения текста 
    oled.clearBuffer();
    oled.sendBuffer();
    digitalWrite(OLED_PWR_PIN, LOW); // Выключение питания дисплея
    Serial.println("Дисплей включился и выключился");

    // ----------INA-----------
    INA226 ina226(0x40);
    if (ina226.begin()) {Serial.println("INA226 инициализирован успешно.");}
    else {Serial.println("Ошибка инициализации INA226.");}

    // -------Wi-Fi----------
    WiFi.begin(settings.wifi_ssid, settings.wifi_password);
    while(WiFi.status() != WL_CONNECTED){ delay(1000); Serial.print(".");}
    Serial.println("");
    Serial.println("WiFi подключен."); 
    Serial.print("IP адрес: "); 
    Serial.println(WiFi.localIP()); // Настройка mDNS 
    MDNS.begin("smart-battery"+String(settings.battery_id));
    
    
}