# Миграция на Arduino Core 3.x

## 🎯 Цель

Этот документ поможет вам перейти с предыдущих версий на новую версию для Arduino Core 3.x с улучшенными проверками состояния Bluetooth.

## 📋 Что изменилось

### 1. Имя файла
- **БЫЛО**: `bluetooth2brushless_hid_client.ino` или `bluetooth2brushless_hid_client_v3.ino`
- **СТАЛО**: `bluetooth2brushless.ino`

### 2. Требования к Arduino Core
- **БЫЛО**: Arduino Core 2.0.0+
- **СТАЛО**: Arduino Core 3.0.0+ (ОБЯЗАТЕЛЬНО!)

### 3. URL менеджера плат
- **БЫЛО**: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- **СТАЛО**: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`

### 4. API изменения
- **LEDC**: `ledcSetup()` + `ledcAttachPin()` → `ledcAttach()`
- **Bluetooth**: Добавлены проверки состояния
- **Диагностика**: Расширенное логирование

## 🚀 Пошаговая миграция

### Шаг 1: Обновление Arduino IDE
```
1. Скачайте Arduino IDE 2.0.0+ с https://www.arduino.cc/en/software
2. Установите и запустите
```

### Шаг 2: Обновление URL менеджера плат
```
1. Файл → Настройки → Дополнительные URL менеджеров плат
2. ЗАМЕНИТЕ старый URL на новый:
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
3. Нажмите OK
```

### Шаг 3: Обновление ESP32 Arduino Core
```
1. Инструменты → Плата → Менеджер плат
2. Найдите "ESP32 by Espressif Systems"
3. Если установлена версия 2.x - удалите её
4. Установите версию 3.0.0 или выше
5. Дождитесь завершения установки
```

### Шаг 4: Настройка платы
```
Инструменты → Плата → ESP32 Arduino → ESP32 Dev Module

Настройки:
- Upload Speed: 921600
- CPU Frequency: 240MHz (WiFi/BT)
- Flash Frequency: 80MHz
- Flash Mode: QIO
- Flash Size: 4MB (32Mb)
- Partition Scheme: Default 4MB with spiffs
- Core Debug Level: Info
- USB CDC On Boot: Disabled (ВАЖНО!)
- USB DFU On Boot: Disabled
- USB Firmware MSC: Disabled
- USB Mode: Hardware CDC and JTAG
```

### Шаг 5: Загрузка нового кода
```
1. Откройте bluetooth2brushless.ino
2. Проверьте MAC адрес пульта (если нужно изменить)
3. Нажмите "Проверить" - должно компилироваться без ошибок
4. Подключите ESP32
5. Выберите правильный порт
6. Нажмите "Загрузить"
```

### Шаг 6: Проверка работы
```
1. Откройте Serial Monitor (115200 baud)
2. Перезагрузите ESP32
3. Должны появиться сообщения о проверке состояния Bluetooth
4. Включите пульт BT13
5. Дождитесь подключения
```

## 🔍 Проверка успешной миграции

### Ожидаемый вывод в Serial Monitor:
```
=== ESP32 HID Host Motor Control v3.x ===
System initialization...
Initializing motor PWM...
PWM initialized successfully
Motor initialization completed
Motor initialized
Checking Bluetooth state...
BT Controller status: 0
BT Controller: IDLE (not initialized)
Bluedroid status: 0
Bluedroid: UNINITIALIZED
Bluetooth state check completed
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
Bluetooth initialized successfully
```

### Признаки успешной миграции:
- ✅ Компиляция без ошибок
- ✅ Сообщения о проверке состояния Bluetooth
- ✅ Успешная инициализация всех компонентов
- ✅ Подключение к пульту BT13
- ✅ Корректная обработка команд

## ⚠️ Возможные проблемы

### "ledcSetup was not declared"
**Причина**: Используется старый код с Arduino Core 2.x API
**Решение**: 
1. Убедитесь, что используете файл `bluetooth2brushless.ino`
2. Проверьте версию Arduino Core (должна быть 3.0.0+)

### "esp_hidh.h: No such file or directory"
**Причина**: Неправильная версия Arduino Core
**Решение**:
1. Проверьте URL менеджера плат
2. Переустановите Arduino Core 3.0.0+
3. Перезапустите Arduino IDE

### "BT_CONTROLLER_INIT_CONFIG_DEFAULT was not declared"
**Причина**: Неполная установка Arduino Core
**Решение**:
1. Полностью удалите старую версию Arduino Core
2. Очистите кэш Arduino IDE
3. Установите Arduino Core 3.0.0+
4. Перезапустите компьютер

### "Bluetooth initialization failed"
**Причина**: Проблемы с инициализацией Bluetooth
**Решение**:
1. Проверьте детальный вывод в Serial Monitor
2. Убедитесь, что ESP32 поддерживает Classic Bluetooth
3. Попробуйте другой ESP32
4. Проверьте качество питания

### Компиляция занимает много времени
**Причина**: Arduino Core 3.x больше и сложнее
**Решение**:
1. Это нормально для первой компиляции
2. Закройте другие программы
3. Используйте SSD для ускорения

## 📊 Сравнение версий

| Функция | Arduino Core 2.x | Arduino Core 3.x |
|---------|------------------|------------------|
| LEDC API | `ledcSetup()` + `ledcAttachPin()` | `ledcAttach()` |
| Проверки Bluetooth | Нет | ✅ Полные проверки |
| Автовосстановление | Нет | ✅ Автоматическое |
| Диагностика | Базовая | ✅ Детальная |
| Мониторинг состояния | Нет | ✅ В реальном времени |
| ESP-IDF версия | 4.4 | 5.1 |
| Стабильность | Хорошая | ✅ Отличная |

## 🎉 Преимущества новой версии

### 1. Надежность
- Проверки состояния Bluetooth перед каждой операцией
- Автоматическое восстановление при сбоях
- Защита от критических ошибок

### 2. Диагностика
- Пошаговое логирование инициализации
- Понятные сообщения о состоянии
- Простая отладка проблем

### 3. Современность
- Использует последние API Arduino Core 3.x
- Совместимость с ESP-IDF 5.1
- Поддержка новых функций ESP32

### 4. Удобство
- Автоматическое управление состоянием
- Минимальное вмешательство пользователя
- Подробная документация

## 📚 Дополнительные ресурсы

### Документация
- `README.md` - Краткое руководство
- `README_v3.md` - Полное руководство пользователя
- `arduino_config_v3.txt` - Детальные настройки Arduino IDE
- `ARDUINO_v3_CHANGELOG.md` - Подробное описание изменений

### Настройки
- Все настройки мотора остаются теми же
- MAC адрес пульта не изменился
- Пины подключения те же (25, 26, 2)

### Поддержка
- Проверьте Serial Monitor для диагностики
- Используйте Core Debug Level = Info для отладки
- Обратитесь к документации при проблемах

## ✅ Контрольный список миграции

- [ ] Обновлен Arduino IDE до 2.0.0+
- [ ] Изменен URL менеджера плат
- [ ] Установлен ESP32 Arduino Core 3.0.0+
- [ ] Настроена плата согласно инструкциям
- [ ] Загружен файл `bluetooth2brushless.ino`
- [ ] Код компилируется без ошибок
- [ ] ESP32 успешно инициализирует Bluetooth
- [ ] Пульт BT13 подключается
- [ ] Команды обрабатываются корректно
- [ ] Автоматические функции работают

## 🎯 Заключение

Миграция на Arduino Core 3.x обеспечивает:
- **Современность**: Последние API и возможности
- **Надежность**: Проверки состояния и автовосстановление
- **Диагностика**: Упрощенная отладка и мониторинг
- **Стабильность**: Улучшенная работа с Bluetooth

После успешной миграции вы получите более стабильную и функциональную систему управления мотором.