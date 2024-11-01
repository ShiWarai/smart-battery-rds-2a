#include "usb_controller/usb_controller.hpp"

void UsbController::clearInputBuffer() {
    while (Serial.available())
        Serial.read();
}

String UsbController::readInput() {
    String str;
    char c;

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

error_t validate_id(String str) {
    int id = str.toInt();

    if(id > 0 && id < 256)
        return 0;
    else
        return 1;
}

error_t validate_uint(String str) {
    int id = str.toInt();

    if(id >= 0)
        if(id == 0 && !str.equals("0")) // На случай, если парсинг неудачный
            return 2;
        else
            return 0;
    else
        return 1; // Число со знаком
}

error_t UsbController::read_String(String *str, error_t validator(String) = nullptr) {
    String buffer = readInput();

    if(validator != nullptr)
    {
        error_t error;
        if(error = validator(buffer) == 0) {
            *str = buffer;
            return error;
        } else {
            Serial.println("Error type: " + String(error));
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
        error_t error;
        if(error = validator(buffer) == 0) {
            *num = buffer.toInt();
            return error;
        } else {
            Serial.println("Error type: " + String(error));
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
        error_t error;
        if(error = validator(buffer) == 0) {
            *num = buffer.toFloat();
            return error;
        } else {
            Serial.println("Error type: " + String(error));
            return error;
        }
    } else 
    {
        *num = buffer.toInt();
        return 0;
    }
}



void UsbController::com_menu() {
	clearInputBuffer();

    uint32_t buffer_num;
    while (true) {
        // Вывод меню
        Serial.println("\n\nМеню:");
        Serial.println("1) Настройки");
        Serial.println("2) Тест");
        Serial.println("3) Сброс");
        Serial.println("0) Выйти");
        
        read_uint32_t(&buffer_num);
        switch (buffer_num) {
            case 1:
                settingsMenu();
                break;
            case 2:
                settingsMenu();
                break;
            case 3:
				nvs_flash_erase();
				nvs_flash_init();
                ESP.restart(); // Перезапуск ESP32
                break;
            default:
				clearInputBuffer();
                return;
        }
    }
}

void UsbController::test(){//тестирование работы всех систем(дисплей, пищалка, вольтамперметр(статистика в консоль), wi-fi(статистика в консоль))

}

#define GENERATE_SERIAL_INPUT_CASE(TYPE, NAME, BUFFER, UPDATE_QUEUE, UPDATE) \
case NAME: \
    Serial.printf("\nВведите новый %s: ", SETTING_NAMES[NAME]);\
    if(read_##TYPE(&BUFFER) != 0) { \
        Serial.println("Ошибка ввода"); \
        break; \
    } \
    update.value = BUFFER; \
    update.key = NAME; \
    xQueueSend(UPDATE_QUEUE, &UPDATE, portMAX_DELAY); \
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
        Serial.println("\n\nМеню\\Настройки:");

        GEN_SETTINGS_OUTPUT_DEFAULT(setting_field, UNIQUE_SETTINGS_TYPES)
        Serial.println("0) Назад");
        
        read_uint32_t(&buffer_uint32_t);
        switch (buffer_uint32_t-1) {
            case SETTING_TYPE::mode:
                Serial.printf("\nВведите новый %s: ", SETTING_NAMES[SETTING_TYPE::mode]);

                if(read_uint32_t(&buffer_uint32_t) != 0) {
                    Serial.println("Ошибка ввода");
                    break;
                }

                update.value = buffer_uint32_t;
                update.key = SETTING_TYPE::mode;

                xQueueSend(settingUpdateQueue, &update, portMAX_DELAY); 
                vTaskDelay(1000);
                ESP.restart();

                break;
            // Генерируем типовые кейсы
            GENERATE_SERIAL_INPUT_CASE(uint32_t, SETTING_TYPE::battery_id, buffer_uint32_t, settingUpdateQueue, update)
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
				com_menu();
			Serial.println(raw_data->getJSON());
        }

		vTaskDelay(settings.usb_delay);
	}
}