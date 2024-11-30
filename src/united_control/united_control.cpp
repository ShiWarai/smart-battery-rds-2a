#include "united_control/united_control.hpp"

void UnitedControl::writeToMemory(std::variant<String, float, uint32_t, nullptr_t> buffer, SETTING_TYPE name) {
    SettingUpdate update;
    update.value = buffer; 
    update.key = name; 
    xQueueSend(settingUpdateQueue, &update, portMAX_DELAY); 
    if(SETTINGS_INFO[name].reboot_is_required) { 
        vTaskDelay(1000); 
        ESP.restart(); 
    } else 
        vTaskDelay(100); 
}

void UnitedControl::startTest(bool needRestart) {
    Preferences pref_test;

    pref_test.begin(TESTING_SPACE_NAME, false);
    pref_test.putBool("test_enabled", true);
    pref_test.end();

    if(needRestart)
        UnitedControl::restartSystem();
}

IntegrationTestResult UnitedControl::readTestResults() {
    return SelfChecking::getIntegrationTestResults();
}

void UnitedControl::restartSystem(time_t delay){
    if(delay > 0)
        vTaskDelay(delay);
        
    ESP.restart();
}