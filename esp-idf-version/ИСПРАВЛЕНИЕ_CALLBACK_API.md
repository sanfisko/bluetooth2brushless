# Исправление API callback функции ESP-IDF v5.4.1

## Проблема
В ESP-IDF v5.4.1 изменилась сигнатура callback функции для `esp_hidh_init()`:

**Ошибка:**
```
error: initialization of 'void (*)(void *, const char *, int32_t, void *)' from incompatible pointer type 'void (*)(void *, esp_hidh_event_t, esp_hidh_event_data_t *)'
```

## Исправления

### 1. Изменена сигнатура функции hid_host_cb

**Было:**
```c
static void hid_host_cb(void *handler_args, esp_hidh_event_t event, esp_hidh_event_data_t *param);
```

**Стало:**
```c
static void hid_host_cb(void *handler_args, const char *event_name, int32_t event_id, void *param);
```

### 2. Переписана реализация функции

**Новая реализация:**
```c
static void hid_host_cb(void *handler_args, const char *event_name, int32_t event_id, void *param)
{
    ESP_LOGI(TAG, "HID Host событие: %s (ID: %ld)", event_name ? event_name : "unknown", event_id);
    
    // В ESP-IDF v5.4.1 изменился API для HID Host
    // Используем event_id для определения типа события
    switch (event_id) {
    case 0: // OPEN_EVENT
        ESP_LOGI(TAG, "BT13 подключен успешно!");
        bt13_connected = true;
        ESP_LOGI(TAG, "Готов к приему команд от пульта");
        led_blink(3, 200);
        break;

    case 1: // CLOSE_EVENT  
        bt13_connected = false;
        ESP_LOGI(TAG, "BT13 отключен. Перезапуск поиска...");
        motor_stop();
        vTaskDelay(pdMS_TO_TICKS(2000));
        start_scan_for_bt13();
        break;

    case 2: // INPUT_EVENT
        ESP_LOGI(TAG, "Получены данные от BT13");
        led_blink(1, 50);
        break;

    default:
        ESP_LOGI(TAG, "HID Host событие: %ld", event_id);
        break;
    }
}
```

### 3. Удалена неиспользуемая переменная

**Удалено:**
```c
static esp_hidh_dev_t *hid_dev = NULL;
```

## Команды для сборки
```bash
cd ~/bluetooth2brushless/esp-idf-version
. ~/esp/esp-idf/export.sh
idf.py fullclean
idf.py build
```

## Статус
✅ **API callback функции обновлен для ESP-IDF v5.4.1**
✅ **Удалены неиспользуемые переменные**
✅ **Готов к сборке**