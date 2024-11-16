#include "test_mode/test_mode.hpp"

void TestMode::test() {
    Preferences pref_test;
    
    pref_test.begin(TESTING_SPACE_NAME, false);
    Serial.begin(115200);

    if(pref_test.isKey("test_enabled") && pref_test.getBool("test_enabled")) {
        // Установка дефолтных значений
        pref_test.putBool("buzzer", false);
        pref_test.putBool("display", false);
        pref_test.putBool("INA226", false);
        pref_test.putBool("wifi", false);
        pref_test.putBool("database", false);

        // Бузер
        pinMode(BUZZER_PIN, OUTPUT);
        tone(BUZZER_PIN, 2560, 250);
        Serial.println("Бузер включился и выключился");
        pref_test.putBool("buzzer", true);

        // OLED дисплей
        pinMode(OLED_PWR_PIN, OUTPUT);
        //Wire.end();
        digitalWrite(OLED_PWR_PIN, HIGH); // Включение питания дисплея
        U8G2_SSD1306_64X32_1F_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);
        oled.begin();
        delay(100);
        oled.setFont(u8g2_font_spleen16x32_mu); 
        oled.drawStr(0, 20, "TEST"); 
        oled.sendBuffer();
        Serial.println("Дисплей включился и выключился");
        pref_test.putBool("display", true);

        // INA226
        INA226 ina226(0x40);
        if (ina226.begin()) {
            Serial.println("INA226 инициализирован успешно.");
            pref_test.putBool("INA226", true);
        }
        else
            Serial.println("Ошибка инициализации INA226.");

        // Кнопка
        pinMode(BUTTONS_PIN, INPUT);

        // Wi-Fi
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);

        // Сканирование доступных сетей
        Serial.println("Сканирование сетей WiFi...");
        int numberOfNetworks = WiFi.scanNetworks();
        
        if (numberOfNetworks == 0) {
            Serial.println("Не найдено сетей");
        } else {
            Serial.printf("Найдено %d:\n", numberOfNetworks);
            // Вывод информации о каждой найденной сети
            for (int i = 0; i < numberOfNetworks; i++) {
                Serial.printf("%d: SSID: %s, RSSI: %d dBm, Encryption: %s\n", 
                    i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), 
                    (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured");
            }

            pref_test.putBool("wifi", true);
        }


        // Тестирование связи с БД
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        for(int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++)
            delay(500);

        if(WiFi.status() == WL_CONNECTED) {
            InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

            if (client.validateConnection())
                pref_test.putBool("database", true);
            else
                Serial.println("Ошибка подключения к БД");

            WiFi.scanDelete();
        }
        WiFi.disconnect(true, true);

        oled.clearDisplay();
        digitalWrite(OLED_PWR_PIN, LOW); // Выключение питания дисплея
        Wire.end();

        pref_test.putBool("test_enabled", false);
        pref_test.end();

        ESP.restart();
    }
    else
        pref_test.end();
}