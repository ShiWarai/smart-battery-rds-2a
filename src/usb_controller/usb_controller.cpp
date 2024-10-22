#include "usb_controller/usb_controller.hpp"


void UsbController::clearInputBuffer() {
    while (Serial.available())
        Serial.read();
}
int UsbController::inputRead(){
    String str;
    while(1){
        while (!Serial.available()); // Ожидание ввода
        char a = Serial.read();
        str+=a;
        if((int)a==13) break;
        else{Serial.print(a);}}
    return str.toInt();
}
void UsbController::com_menu() {
	clearInputBuffer();

    
    while (true) {
        // Вывод меню
        Serial.println("\n\nМеню:");
        Serial.println("1) Настройки");
        Serial.println("2) Сброс");
        Serial.println("0) Выйти");
        
        
        switch (inputRead()) {
            case 1:
                settings_menu();
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

void UsbController::settings_menu() {
    int setting_choice;
    DECLARE_SETTING_TYPES_VARIANT(UNIQUE_SETTINGS_TYPES) buffer;

	SettingUpdate update;

    while (true) {
        // Вывод меню настроек
        Serial.println("\n\nМеню\\Настройки:");
        Serial.println("1) ID");
        Serial.println("0) Назад");
        
        switch (inputRead()) {
            case 1:
                Serial.print("\nВведите новый ID: ");

                update.value = (uint32_t)inputRead();
                update.key = SETTING_TYPE::battery_id;

                xQueueSend(settingUpdateQueue, &update, portMAX_DELAY);
                vTaskDelay(10);
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