#include "test_mode/test_mode.hpp"

void TestMode::test() {
    Preferences pref_test;
    pref_test.begin("testing", false);
    
    if(pref_test.getBool("test_enabled")) {
        Serial.begin(115200);
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
        
        Serial.println("Дисплей включился и выключился");

        // ----------INA-----------
        INA226 ina226(0x40);
        if (ina226.begin()) {Serial.println("INA226 инициализирован успешно.");}
        else {Serial.println("Ошибка инициализации INA226.");}

        // -------Wi-Fi----------
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);
        Serial.println("Scanning for networks...");
        // Сканирование доступных сетей
        int numberOfNetworks = WiFi.scanNetworks();
        
        if (numberOfNetworks == 0) {
            Serial.println("No networks found.");
        } else {
            Serial.printf("Found %d networks:\n", numberOfNetworks);
            // Вывод информации о каждой найденной сети
            for (int i = 0; i < numberOfNetworks; i++) {
            Serial.printf("%d: SSID: %s, RSSI: %d dBm, Encryption: %s\n", 
                            i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), 
                            (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured");
            }
        }
        // Заканчиваем сканирование
        WiFi.scanDelete();
        WiFi.disconnect(true);
        
        pref_test.putBool("test_enabled", false);

        oled.clearDisplay();
        digitalWrite(OLED_PWR_PIN, LOW); // Выключение питания дисплея
        Wire.end();
    }

    pref_test.end();
}