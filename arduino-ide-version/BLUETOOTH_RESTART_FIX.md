# 🔧 Исправление бесконечного перезапуска Bluetooth

## ❌ Проблема
ESP32 попадал в бесконечный цикл попыток инициализации Bluetooth с ошибкой:
```
WARNING: Bluetooth not enabled, attempting restart...
Starting Bluetooth initialization...
Initializing BT controller...
BT controller init error: ESP_ERR_INVALID_STATE
```

## 🔍 Причина
1. **Повторная инициализация**: Код пытался инициализировать уже инициализированный Bluetooth контроллер
2. **Отсутствие проверок состояния**: Не проверялось, что компоненты уже инициализированы
3. **Бесконечные попытки**: Отсутствовало ограничение на количество попыток перезапуска

## ✅ Решение

### 1. Улучшенная проверка состояния
```cpp
// Если Bluetooth уже полностью инициализирован, просто возвращаем успех
if (controller_status == ESP_BT_CONTROLLER_STATUS_ENABLED && 
    bluedroid_status == ESP_BLUEDROID_STATUS_ENABLED) {
    Serial.println("Bluetooth already fully initialized");
    bluetooth_initialized = true;
    bluetooth_enabled = true;
    return true;
}
```

### 2. Предотвращение повторной инициализации
```cpp
// Инициализация контроллера только если он не инициализирован
if (controller_status == ESP_BT_CONTROLLER_STATUS_IDLE) {
    // Инициализация...
} else if (controller_status == ESP_BT_CONTROLLER_STATUS_ENABLED) {
    Serial.println("BT controller already enabled");
}
```

### 3. Ограничение попыток перезапуска
```cpp
// Переменные для ограничения попыток перезапуска
static uint32_t last_restart_attempt = 0;
static int restart_attempts = 0;
static const int MAX_RESTART_ATTEMPTS = 3;
static const uint32_t RESTART_COOLDOWN_MS = 30000; // 30 секунд между попытками
```

### 4. Защита от повторной регистрации
```cpp
// Регистрация GAP callback только если еще не зарегистрирован
if (!gap_callback_registered) {
    // Регистрация...
    gap_callback_registered = true;
} else {
    Serial.println("GAP callback already registered");
}
```

## 🎯 Результат

### Ожидаемое поведение после исправления:
```
=== ESP32 HID Host Motor Control v3.x ===
System initialization...
Checking Bluetooth state...
BT Controller status: 2
BT Controller: ENABLED
Bluedroid status: 2
Bluedroid: ENABLED
Starting Bluetooth initialization...
Bluetooth already fully initialized
Bluetooth initialized successfully
```

### При первом запуске:
```
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
GAP callback registered
Initializing HID Host...
HID Host initialized
Bluetooth initialization completed successfully
```

### При попытках перезапуска:
```
WARNING: Bluetooth not enabled, attempting restart... (attempt 1/3)
Starting Bluetooth initialization...
Bluetooth already fully initialized
Bluetooth restarted successfully
```

### При превышении лимита попыток:
```
WARNING: Bluetooth not enabled, attempting restart... (attempt 3/3)
Bluetooth restart failed (attempt 3/3)
[Ожидание 30 секунд перед сбросом счетчика]
Restart attempts counter reset after cooldown
```

## 🔧 Технические детали

### Добавленные переменные состояния:
- `gap_callback_registered` - флаг регистрации GAP callback
- `hidh_initialized` - флаг инициализации HID Host
- `last_restart_attempt` - время последней попытки перезапуска
- `restart_attempts` - счетчик попыток перезапуска

### Константы ограничения:
- `MAX_RESTART_ATTEMPTS = 3` - максимум 3 попытки подряд
- `RESTART_COOLDOWN_MS = 30000` - 30 секунд перед сбросом счетчика

### Проверяемые состояния:
- `ESP_BT_CONTROLLER_STATUS_IDLE` - не инициализирован
- `ESP_BT_CONTROLLER_STATUS_INITED` - инициализирован, но не включен
- `ESP_BT_CONTROLLER_STATUS_ENABLED` - полностью готов
- `ESP_BLUEDROID_STATUS_UNINITIALIZED` - не инициализирован
- `ESP_BLUEDROID_STATUS_INITIALIZED` - инициализирован, но не включен
- `ESP_BLUEDROID_STATUS_ENABLED` - полностью готов

## 🚀 Преимущества исправления

1. **Стабильность**: Нет бесконечных циклов перезапуска
2. **Эффективность**: Избегание повторной инициализации
3. **Диагностика**: Понятные сообщения о состоянии
4. **Надежность**: Ограничение попыток предотвращает зависание
5. **Восстановление**: Автоматический сброс после периода ожидания

## 📋 Для пользователей

### Перед использованием:
1. **Убедитесь в правильном URL**: Файл → Настройки → Дополнительные ссылки:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
2. **Установите ESP32 Core 3.0.0+**

### После обновления кода:
1. Перезагрузите ESP32
2. Откройте Serial Monitor (115200 baud)
3. Проверьте, что нет повторяющихся ошибок
4. Убедитесь, что Bluetooth инициализируется корректно
5. Проверьте подключение к пульту BT13

## ⚠️ Если проблема остается

1. **Полная перезагрузка**: Отключите питание ESP32 на 10 секунд
2. **Проверка питания**: Убедитесь в стабильном питании 3.3V
3. **Другой ESP32**: Попробуйте другую плату ESP32
4. **Заводские настройки**: Очистите flash память ESP32

## 🔄 Совместимость

Исправление совместимо с:
- ✅ Arduino Core 3.x
- ✅ ESP-IDF 5.1+
- ✅ Всеми моделями ESP32 с Classic Bluetooth
- ✅ Существующими настройками проекта