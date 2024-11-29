#include "usb_controller/usb_controller.hpp"

void UsbController::clearInputBuffer() {
    while (Serial.available())
        Serial.read();
}

String UsbController::readInput() {
    String str;
    char c;

    clearInputBuffer();
    while (1)
    {
        while (!Serial.available()) vTaskDelay(1); // Ожидание ввода

        c = Serial.read();
        
        if (c == '\r')
            break;
        else
            Serial.print(c);

        str += c;
    }

    return str;
}

error_t UsbController::read_String(String *str, error_t validator(String) = nullptr) {
    String buffer = readInput();

    if(validator != nullptr)
    {
        error_t error = validator(buffer);
        if(error == 0) {
            buffer.trim();
            *str = buffer;
            return error;
        } else {
            Serial.println("\r\nОшибка: " + String(error));
            return error;
        }
    } else {
        buffer.trim();
        *str = buffer;
        return 0;
    }
}

error_t UsbController::read_uint32_t(uint32_t *num, error_t validator(String) = nullptr) {
    String buffer = readInput();

    if(validator != nullptr)
    {
        error_t error = validator(buffer);
        if(error == 0) {
            *num = buffer.toInt();
            return error;
        } else {
            Serial.println("\r\nОшибка: " + String(error));
            return error;
        }
    } else {
        *num = buffer.toInt();
        return 0;
    }
}

error_t UsbController::read_float(float *num, error_t validator(String) = nullptr) {
    String buffer = readInput();

    if(validator != nullptr)
    {
        error_t error = validator(buffer);
        if(error == 0) {
            *num = buffer.toFloat();
            return error;
        } else {
            Serial.println("\r\nОшибка: " + String(error));
            return error;
        }
    } else 
    {
        *num = buffer.toFloat();
        return 0;
    }
}

void UsbController::comMenu() {
	clearInputBuffer();
    Preferences pref_test;
    uint32_t buffer_num;
    while (true) {
        // Вывод меню
        Serial.println("\r\nМеню:");
        Serial.println("1) Настройки");
        Serial.println("2) Вывести информацию о системе");
        Serial.println("3) Тест");
        Serial.println("4) Результаты теста");
        Serial.println("5) Рестарт");
        Serial.println("6) Сброс(очистка памяти + рестарт)");
        Serial.println("0) Выйти");
        
        IntegrationTestResult results = UnitedControl::readTestResults();
        
        read_uint32_t(&buffer_num);
        switch (buffer_num) {
            case 1:
                settingsMenu();
                break;
            case 2:
                outputInfo();
                break;
            case 3:
                UnitedControl::startTest();
                break;
            case 4:
                Serial.println("\r\nРезультаты тестирования:");

                Serial.print("Buzzer integrationTest: "); Serial.println(results.buzzerTest ? "Passed" : "Failed"); 
                Serial.print("Display integrationTest: "); Serial.println(results.displayTest ? "Passed" : "Failed"); 
                Serial.print("INA226 integrationTest: "); Serial.println(results.ina226Test ? "Passed" : "Failed"); 
                Serial.print("WiFi integrationTest: "); Serial.println(results.wifiTest ? "Passed" : "Failed"); 
                Serial.print("Database integrationTest: "); Serial.println(results.databaseTest ? "Passed" : "Failed");
                break;
            case 5:
                ESP.restart();
                break;
            case 6:
				nvs_flash_erase();
				nvs_flash_init();
                ESP.restart();
                break;
            default:
                Serial.print("\r\n");
				clearInputBuffer();
                return;
        }
    }
}

void UsbController::outputInfo() {
    Serial.printf("\r\nАккумулятор #%d\r\n", settings.battery_id);

    Serial.printf("Текущий IP: %s\r\n", WiFi.localIP().toString());
}

void UsbController::settingsMenu() {
    DECLARE_SETTING_TYPES_VARIANT(UNIQUE_SETTINGS_TYPES) buffer;
    DECLARE_SETTING_TYPES_LINKS_VARIANT(UNIQUE_SETTINGS_TYPES) setting_field;

	SettingUpdate update;

    while (true) {
        // Вывод меню настроек
        Serial.println("\r\nМеню\\Настройки:");

        GEN_SETTINGS_OUTPUT_DEFAULT(setting_field, UNIQUE_SETTINGS_TYPES)
        Serial.println("0) Назад");

        GENERATE_SERIAL_INPUTS(settingUpdateQueue, update, SETTINGS_FIELDS)
    }
}

void UsbController::usbTask(void *pvParameters) {\
	while(true) {
		if(Serial.isConnected())
		{
			if(Serial.available()) // Позже сделаем возможность прерывать поток
				comMenu();
			Serial.println(raw_data->getJSON());
        }

		vTaskDelay(settings.usb_delay);
	}
}