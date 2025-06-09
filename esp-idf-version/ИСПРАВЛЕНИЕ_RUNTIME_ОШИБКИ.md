# Исправление Runtime ошибки ESP-IDF v5.4.1

## Проблема
После успешной компиляции ESP32 перезагружается с ошибкой:
```
ESP_ERROR_CHECK failed: esp_err_t 0x102 (ESP_ERR_INVALID_ARG) at 0x400d927a
expression: esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)
```

## Причина
В ESP-IDF v5.4.1 изменились требования к конфигурации Bluetooth контроллера. Конфликт между настройками sdkconfig и кодом инициализации.

## Исправления

### 1. Обновлена конфигурация sdkconfig.defaults

**Изменено:**
```
# Режим контроллера - только Classic BT для HID Host
# CONFIG_BTDM_CTRL_MODE_BTDM=y
# CONFIG_BTDM_CTRL_MODE_BLE_ONLY=n
CONFIG_BTDM_CTRL_MODE_BR_EDR_ONLY=y

# Отключить BLE для экономии памяти
CONFIG_BT_BLE_ENABLED=n
```

### 2. Добавлена подробная обработка ошибок

**Заменено:**
```c
ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
ESP_ERROR_CHECK(esp_bluedroid_init());
ESP_ERROR_CHECK(esp_bluedroid_enable());
```

**На:**
```c
ESP_LOGI(TAG, "Инициализация BT контроллера...");
ret = esp_bt_controller_init(&bt_cfg);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Ошибка инициализации BT контроллера: %s", esp_err_to_name(ret));
    return;
}

ESP_LOGI(TAG, "Включение BT контроллера...");
ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Ошибка включения BT контроллера: %s", esp_err_to_name(ret));
    return;
}

ESP_LOGI(TAG, "Инициализация Bluedroid...");
ret = esp_bluedroid_init();
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Ошибка инициализации Bluedroid: %s", esp_err_to_name(ret));
    return;
}

ESP_LOGI(TAG, "Включение Bluedroid...");
ret = esp_bluedroid_enable();
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Ошибка включения Bluedroid: %s", esp_err_to_name(ret));
    return;
}
```

## Команды для тестирования

```bash
cd ~/bluetooth2brushless/esp-idf-version
. ~/esp/esp-idf/export.sh
idf.py fullclean
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Ожидаемый результат
После исправлений ESP32 должен успешно инициализировать Bluetooth без перезагрузок и показать логи:
```
I (xxx) BT13_MOTOR_CONTROL: Инициализация BT контроллера...
I (xxx) BT13_MOTOR_CONTROL: Включение BT контроллера...
I (xxx) BT13_MOTOR_CONTROL: Инициализация Bluedroid...
I (xxx) BT13_MOTOR_CONTROL: Включение Bluedroid...
```

## Статус
✅ **Конфигурация Bluetooth обновлена для ESP-IDF v5.4.1**
✅ **Добавлена подробная диагностика ошибок**
✅ **Готов к тестированию**