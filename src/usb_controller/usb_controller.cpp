#include "usb_controller/usb_controller.hpp"


#define DECLARE_SERIAL_PRINT_ITER(TYPE, F1, F2, SETTING_POINTER) \
if(SETTING_TYPES[i] == #TYPE) \
{ \
    Serial.print(SETTING_NAMES[i]); \
    Serial.print("\r\t\t\t="); \
    Serial.print(*(std::get<TYPE*>(SETTING_POINTER))); \
}

#define GEN_SETTINGS_OUTPUT_DEFAULT(SETTING_POINTER, TYPES) \
for (unsigned short i = 0; i < SETTING_TYPE::SETTINGS_COUNT; i++) { \
    SETTING_POINTER = getSettingFieldPointer(i); \
    Serial.print(String(i+1)+") "); \
    TYPES(DECLARE_SERIAL_PRINT_ITER, SETTING_POINTER) \
    Serial.println(); \
}

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
        Serial.println("2) Тест");
        Serial.println("3) Сброс");
        Serial.println("0) Выйти");
        
        
        switch (readUInt32(validate_uint)) {
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

void UsbController::settingsMenu() {
    int setting_choice;
    DECLARE_SETTING_TYPES_VARIANT(UNIQUE_SETTINGS_TYPES) buffer;

	SettingUpdate update;

    while (true) {
        // Вывод меню настроек
        Serial.println("\n\nМеню\\Настройки:");
        DECLARE_SETTING_TYPES_LINKS_VARIANT(UNIQUE_SETTINGS_TYPES) setting_field;
        GEN_SETTINGS_OUTPUT_DEFAULT(setting_field, UNIQUE_SETTINGS_TYPES)
        
        // Serial.print("1) ID             =");Serial.println(settings.battery_id);
        // Serial.print("2) wifi_ssid      =");Serial.println(settings.wifi_ssid);
        // Serial.print("3) wifi_password  =");Serial.println(settings.wifi_password);
        // Serial.print("4) sensor_delay   =");Serial.println(settings.sensor_delay);
        // Serial.print("5) usb_delay      =");Serial.println(settings.usb_delay);
        // Serial.print("6) wireless_delay =");Serial.println(settings.wireless_delay);
        // Serial.print("7) display_time   =");Serial.println(settings.display_time);
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
            case 2:
                Serial.print("\nВведите новый wifi_ssid: ");

                update.value = readString(0);
                update.key = SETTING_TYPE::wifi_ssid;

                if(std::get<uint32_t>(update.value) == 0)
                    break;

                xQueueSend(settingUpdateQueue, &update, portMAX_DELAY);
                break;
            case 3:
                Serial.print("\nВведите новый wifi_password: ");

                update.value = readString(0);
                update.key = SETTING_TYPE::wifi_password;

                if(std::get<uint32_t>(update.value) == 0)
                    break;

                xQueueSend(settingUpdateQueue, &update, portMAX_DELAY);
                break;
            case 4:
                Serial.print("\nВведите новый sensor_delay: ");

                update.value = readUInt32(validate_uint);
                update.key = SETTING_TYPE::sensor_delay;

                if(std::get<uint32_t>(update.value) == 0)
                    break;

                xQueueSend(settingUpdateQueue, &update, portMAX_DELAY);
                break;
            case 5:
                Serial.print("\nВведите новый usb_delay: ");

                update.value = readUInt32(validate_uint);
                update.key = SETTING_TYPE::usb_delay;

                if(std::get<uint32_t>(update.value) == 0)
                    break;

                xQueueSend(settingUpdateQueue, &update, portMAX_DELAY);
                break;
            case 6:
                Serial.print("\nВведите новый wireless_delay: ");

                update.value = readUInt32(validate_uint);
                update.key = SETTING_TYPE::wireless_delay;

                if(std::get<uint32_t>(update.value) == 0)
                    break;

                xQueueSend(settingUpdateQueue, &update, portMAX_DELAY);
                break;
            case 7:
                Serial.print("\nВведите новый display_time: ");

                update.value = readUInt32(validate_uint);
                update.key = SETTING_TYPE::display_time;

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