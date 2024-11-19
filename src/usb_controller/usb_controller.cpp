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
        *num = buffer.toInt();
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
                Serial.println("Результаты тестирования:");

                Serial.print("Buzzer test: "); Serial.println(results.buzzerTest ? "Passed" : "Failed"); 
                Serial.print("Display test: "); Serial.println(results.displayTest ? "Passed" : "Failed"); 
                Serial.print("INA226 test: "); Serial.println(results.ina226Test ? "Passed" : "Failed"); 
                Serial.print("WiFi test: "); Serial.println(results.wifiTest ? "Passed" : "Failed"); 
                Serial.print("Database test: "); Serial.println(results.databaseTest ? "Passed" : "Failed");
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
    Serial.printf("Текущая частота: %d\r\n", getCpuFrequencyMhz());
}

#define GENERATE_SERIAL_INPUT_CASE(TYPE, NAME, VALIDATOR_FUNC, BUFFER, UPDATE_QUEUE, UPDATE) \
case NAME: \
    Serial.printf("\r\nВведите новый %s: ", SETTINGS_INFO[NAME].name);\
    if(read_##TYPE(&BUFFER, VALIDATOR_FUNC) != 0) { \
        Serial.println("\r\nОшибка ввода"); \
        break; \
    } \
    update.value = BUFFER; \
    update.key = NAME; \
    xQueueSend(UPDATE_QUEUE, &UPDATE, portMAX_DELAY); \
    if(SETTINGS_INFO[NAME].reboot_is_required) { \
        vTaskDelay(1000); \
        ESP.restart(); \
    } else \
        vTaskDelay(100); \
    break;

void UsbController::settingsMenu() {
    uint32_t buffer_uint32_t;
    float buffer_float;
    String buffer_String;

    DECLARE_SETTING_TYPES_VARIANT(UNIQUE_SETTINGS_TYPES) buffer;
    DECLARE_SETTING_TYPES_LINKS_VARIANT(UNIQUE_SETTINGS_TYPES) setting_field;

	SettingUpdate update;

    while (true) {
        // Вывод меню настроек
        Serial.println("\r\nМеню\\Настройки:");

        GEN_SETTINGS_OUTPUT_DEFAULT(setting_field, UNIQUE_SETTINGS_TYPES)
        Serial.println("0) Назад");
        
        read_uint32_t(&buffer_uint32_t);
        switch (buffer_uint32_t-1) {
            // Генерируем типовые кейсы
            GENERATE_SERIAL_INPUT_CASE(String, SETTING_TYPE::access_key, nullptr, buffer_String, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(uint32_t, SETTING_TYPE::sensor_delay, validate_uint, buffer_uint32_t, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(uint32_t, SETTING_TYPE::usb_delay, validate_uint, buffer_uint32_t, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(uint32_t, SETTING_TYPE::wireless_delay, validate_uint, buffer_uint32_t, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(uint32_t, SETTING_TYPE::display_time, validate_uint, buffer_uint32_t, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(String, SETTING_TYPE::wifi_ssid, nullptr, buffer_String, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(String, SETTING_TYPE::wifi_password, nullptr, buffer_String, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(String, SETTING_TYPE::influxdb_url, nullptr, buffer_String, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(String, SETTING_TYPE::influxdb_org, nullptr, buffer_String, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(String, SETTING_TYPE::influxdb_bucket, nullptr, buffer_String, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(String, SETTING_TYPE::influxdb_token, nullptr, buffer_String, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(uint32_t, SETTING_TYPE::battery_id, validate_id, buffer_uint32_t, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(String, SETTING_TYPE::hostname, nullptr, buffer_String, settingUpdateQueue, update)
            GENERATE_SERIAL_INPUT_CASE(uint32_t, SETTING_TYPE::mode, validate_uint, buffer_uint32_t, settingUpdateQueue, update)
            default:
                clearInputBuffer();
				return;
        }
    }
}

void UsbController::usbTask(void *pvParameters) {
	Serial.begin(115200);

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