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

String UsbController::readString(error_t validate(String)) {
    String str = readInput();

    error_t error;
    if(error = validate(str) == 0) {
        return str;
    } else {
        Serial.println("Error type: " + String(error));
        return "";
    }
}

uint32_t UsbController::readUInt32(error_t validate(String)) {
    String str = readInput();

    error_t error;
    if(error = validate(str) == 0) {
        return str.toInt();
    } else {
        Serial.println("Error type: " + String(error));
        return 0;
    }
}

float UsbController::readFloat(error_t validate(String)) {
    String str = readInput();

    error_t error;
    if(error = validate(str) == 0) {
        return str.toFloat();
    } else {
        Serial.println("\nError type: " + String(error));
        return 0;
    }
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

void UsbController::com_menu() {
	clearInputBuffer();

    
    while (true) {
        // Вывод меню
        Serial.println("\n\nМеню:");
        Serial.println("1) Настройки");
        Serial.println("2) Сброс");
        Serial.println("0) Выйти");
        
        
        switch (readUInt32(validate_uint)) {
            case 1:
                settingsMenu();
                break;
            case 2:
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

void UsbController::settingsMenu() {
    int setting_choice;
    DECLARE_SETTING_TYPES_VARIANT(UNIQUE_SETTINGS_TYPES) buffer;

	SettingUpdate update;

    while (true) {
        // Вывод меню настроек
        Serial.println("\n\nМеню\\Настройки:");
        Serial.println("1) ID");
        Serial.println("0) Назад");
        
        switch (readUInt32(validate_uint)) {
            case 1:
                Serial.print("\nВведите новый ID: ");

                update.value = readUInt32(validate_id);
                update.key = SETTING_TYPE::battery_id;

                if(std::get<uint32_t>(update.value) == 0)
                    break;

                xQueueSend(settingUpdateQueue, &update, portMAX_DELAY);
                break;
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