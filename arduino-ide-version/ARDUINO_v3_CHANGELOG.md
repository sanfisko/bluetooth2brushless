# Arduino Core 3.x версия - Обновление с проверками состояния Bluetooth

## 🔄 Обзор изменений

Версия для Arduino Core 3.x была обновлена с учетом изменений API в ESP32 Arduino Core 3.0.0+ (основан на ESP-IDF 5.1) и добавлены комплексные проверки состояния Bluetooth перед инициализацией.

## ✨ Ключевые улучшения

### 1. Обновление для Arduino Core 3.x API
- **LEDC API**: Обновлен с `ledcSetup()` + `ledcAttachPin()` на `ledcAttach()`
- **Bluetooth API**: Совместимость с ESP-IDF 5.1
- **Новые функции**: Поддержка Peripheral Manager
- **Результат**: Полная совместимость с современным Arduino Core

### 2. Проверки состояния Bluetooth
- **Добавлено**: `check_bluetooth_state()` - проверка состояния перед инициализацией
- **Добавлено**: `initialize_bluetooth()` - умная инициализация с проверками
- **Добавлено**: Мониторинг состояния в реальном времени
- **Результат**: Надежная инициализация и работа Bluetooth

### 3. Улучшенная диагностика
- **Добавлено**: Детальное логирование состояния контроллера Bluetooth
- **Добавлено**: Проверка состояния Bluedroid
- **Добавлено**: Пошаговая диагностика инициализации
- **Результат**: Простая отладка и решение проблем

### 4. Автоматическое восстановление
- **Добавлено**: Проверка состояния Bluetooth в loop()
- **Добавлено**: Автоматический перезапуск при сбоях
- **Добавлено**: Защита от повторной инициализации
- **Результат**: Стабильная работа без вмешательства пользователя

## 📋 Детальные изменения

### API изменения для Arduino Core 3.x

#### LEDC (PWM) API
```cpp
// БЫЛО (Arduino Core 2.x):
#define PWM_CHANNEL         0
ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
ledcAttachPin(MOTOR_SPEED_PIN, PWM_CHANNEL);
ledcWrite(PWM_CHANNEL, actual_speed);

// СТАЛО (Arduino Core 3.x):
// Каналы теперь автоматически назначаются пинам
ledcAttach(MOTOR_SPEED_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
ledcWrite(MOTOR_SPEED_PIN, actual_speed);
```

#### Заголовочные файлы
```cpp
// Обновлены для совместимости с ESP-IDF 5.1:
#include <Arduino.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_hidh.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
```

### Новые функции проверки состояния Bluetooth

#### Проверка состояния
```cpp
static bool check_bluetooth_state(void)
{
    Serial.println("Checking Bluetooth state...");
    
    // Проверка состояния контроллера Bluetooth
    esp_bt_controller_status_t controller_status = esp_bt_controller_get_status();
    Serial.printf("BT Controller status: %d\n", controller_status);
    
    switch (controller_status) {
        case ESP_BT_CONTROLLER_STATUS_IDLE:
            Serial.println("BT Controller: IDLE (not initialized)");
            break;
        case ESP_BT_CONTROLLER_STATUS_INITED:
            Serial.println("BT Controller: INITIALIZED");
            break;
        case ESP_BT_CONTROLLER_STATUS_ENABLED:
            Serial.println("BT Controller: ENABLED");
            break;
        default:
            Serial.println("BT Controller: INVALID STATUS");
            return false;
    }
    
    // Проверка состояния Bluedroid
    esp_bluedroid_status_t bluedroid_status = esp_bluedroid_get_status();
    Serial.printf("Bluedroid status: %d\n", bluedroid_status);
    
    switch (bluedroid_status) {
        case ESP_BLUEDROID_STATUS_UNINITIALIZED:
            Serial.println("Bluedroid: UNINITIALIZED");
            break;
        case ESP_BLUEDROID_STATUS_INITIALIZED:
            Serial.println("Bluedroid: INITIALIZED");
            break;
        case ESP_BLUEDROID_STATUS_ENABLED:
            Serial.println("Bluedroid: ENABLED");
            break;
        default:
            Serial.println("Bluedroid: UNKNOWN STATUS");
            break;
    }
    
    Serial.println("Bluetooth state check completed");
    return true;
}
```

#### Умная инициализация
```cpp
static bool initialize_bluetooth(void)
{
    esp_err_t ret;
    
    Serial.println("Starting Bluetooth initialization...");
    
    // Проверяем текущее состояние контроллера
    esp_bt_controller_status_t controller_status = esp_bt_controller_get_status();
    
    if (controller_status == ESP_BT_CONTROLLER_STATUS_IDLE) {
        // Инициализация контроллера только если нужно
        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        
        Serial.println("Initializing BT controller...");
        ret = esp_bt_controller_init(&bt_cfg);
        if (ret != ESP_OK) {
            Serial.printf("BT controller init error: %s\n", esp_err_to_name(ret));
            return false;
        }
        Serial.println("BT controller initialized");
    }
    
    // Аналогично для включения контроллера и Bluedroid...
    
    bluetooth_initialized = true;
    bluetooth_enabled = true;
    
    Serial.println("Bluetooth initialization completed successfully");
    return true;
}
```

### Мониторинг состояния в реальном времени

#### В основном цикле
```cpp
void loop() {
    // Проверка состояния Bluetooth
    if (!bluetooth_enabled) {
        Serial.println("WARNING: Bluetooth not enabled, attempting restart...");
        if (initialize_bluetooth()) {
            Serial.println("Bluetooth restarted successfully");
        } else {
            delay(5000); // Ждем 5 секунд перед повторной попыткой
            return;
        }
    }
    
    // Остальная логика...
}
```

#### В задаче мониторинга
```cpp
static void connection_monitor_task(void *pvParameters)
{
    while (1) {
        // Проверка состояния Bluetooth
        if (bluetooth_enabled) {
            esp_bt_controller_status_t controller_status = esp_bt_controller_get_status();
            esp_bluedroid_status_t bluedroid_status = esp_bluedroid_get_status();
            
            if (controller_status != ESP_BT_CONTROLLER_STATUS_ENABLED || 
                bluedroid_status != ESP_BLUEDROID_STATUS_ENABLED) {
                Serial.println("WARNING: Bluetooth state changed, marking as disabled");
                bluetooth_enabled = false;
            }
        }
        
        // Остальная логика мониторинга...
    }
}
```

### Улучшенная обработка ошибок

#### Защита от повторной инициализации
```cpp
static void start_scan_for_bt13(void)
{
    if (!bluetooth_enabled) {
        Serial.println("Cannot start scan: Bluetooth not enabled");
        return;
    }

    if (scanning_in_progress) {
        Serial.println("Scan already in progress, skipping...");
        return;
    }

    if (bt13_connected) {
        Serial.println("BT13 already connected, scan not needed");
        return;
    }
    
    // Запуск сканирования...
}
```

## 🎯 Результаты

### Функциональность
- ✅ Полная совместимость с Arduino Core 3.x API
- ✅ Проверка состояния Bluetooth перед каждой операцией
- ✅ Автоматическое восстановление при сбоях
- ✅ Детальная диагностика состояния системы
- ✅ Защита от повторной инициализации

### Надежность
- ✅ Мониторинг состояния Bluetooth в реальном времени
- ✅ Умная инициализация только необходимых компонентов
- ✅ Автоматический перезапуск при обнаружении проблем
- ✅ Защита от критических ошибок

### Диагностика
- ✅ Пошаговое логирование инициализации
- ✅ Понятные сообщения о состоянии системы
- ✅ Детальная информация об ошибках
- ✅ Простая отладка проблем

## 🔧 Миграция

### С Arduino Core 2.x на 3.x:
1. **Обновите URL менеджера плат**:
   - Старый: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Новый: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`

2. **Установите ESP32 Arduino Core 3.0.0+**:
   - Инструменты → Плата → Менеджер плат
   - Найдите "ESP32" → Обновите до версии 3.0.0+

3. **Замените файл**:
   - Используйте `bluetooth2brushless.ino`
   - Обновите настройки согласно `arduino_config_v3.txt`

4. **Проверьте настройки платы**:
   - Добавились новые параметры USB
   - Убедитесь, что USB CDC On Boot = Disabled

### Настройки остаются теми же:
- MAC адрес пульта BT13
- Пины подключения (25, 26, 2)
- Логика управления двигателем
- HID Usage коды

## 🚀 Новые возможности

### Диагностические сообщения
```
=== ESP32 HID Host Motor Control v3.x ===
Checking Bluetooth state...
BT Controller status: 0
BT Controller: IDLE (not initialized)
Bluedroid status: 0
Bluedroid: UNINITIALIZED
Starting Bluetooth initialization...
Initializing BT controller...
BT controller initialized
Enabling BT controller...
BT controller enabled
Initializing Bluedroid...
Bluedroid initialized
Enabling Bluedroid...
Bluedroid enabled
Registering GAP callback...
Initializing HID Host...
Bluetooth initialization completed successfully
```

### Предупреждения и восстановление
```
WARNING: Bluetooth state changed, marking as disabled
WARNING: Bluetooth not enabled, attempting restart...
Bluetooth restarted successfully
Cannot start scan: Bluetooth not enabled
```

### Улучшенная инициализация мотора
```
Initializing motor PWM...
PWM initialized successfully
Motor initialization completed
```

## 📚 Документация

### Обновленные файлы
- `bluetooth2brushless.ino` - обновлен для Arduino Core 3.x
- `arduino_config_v3.txt` - новые настройки и требования
- `ARDUINO_v3_CHANGELOG.md` - этот файл с описанием изменений

### Новые функции
- Проверка состояния Bluetooth перед инициализацией
- Мониторинг состояния в реальном времени
- Автоматическое восстановление при сбоях
- Детальная диагностика ошибок
- Защита от повторной инициализации

## 🔍 Отладка

### Типичные проблемы и решения

#### "ledcSetup was not declared"
**Причина**: Используется старый API Arduino Core 2.x
**Решение**: Обновите до Arduino Core 3.x и используйте `ledcAttach()`

#### "Bluetooth initialization failed"
**Причина**: Проблемы с инициализацией Bluetooth
**Решение**: Проверьте вывод диагностики в Serial Monitor

#### "Cannot start scan: Bluetooth not enabled"
**Причина**: Bluetooth не инициализирован или отключился
**Решение**: Система автоматически попытается восстановить соединение

## 🚀 Заключение

Версия для Arduino Core 3.x представляет собой значительное улучшение:

- **Современность**: Использует последние API Arduino Core 3.x
- **Надежность**: Комплексные проверки состояния Bluetooth
- **Диагностика**: Детальное логирование для упрощения отладки
- **Автоматизация**: Автоматическое восстановление при сбоях
- **Совместимость**: Полная поддержка ESP-IDF 5.1

Это делает версию 3.x наиболее стабильной и функциональной для современных проектов с ESP32.