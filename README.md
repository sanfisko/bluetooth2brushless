# bluetooth2brushless
ESP32 Brushless DC Motor Control with Bluetooth Remote BT13

## 🔄 ОБНОВЛЕНИЕ: HID Client версия

**Проблема решена!** Создана новая версия, где ESP32 подключается к пульту BT13 как HID клиент.

### Доступные версии:
1. **`bluetooth2brushless.ino`** - Оригинальная версия (ESP32 как сервер)
2. **`bluetooth2brushless_hid_client.ino`** - Arduino IDE версия (ESP32 как HID клиент) 
3. **`esp-idf-version/`** - ESP-IDF версия (рекомендуется для продакшена)

### Быстрый старт с BT13:
```bash
# Для ESP-IDF (рекомендуется)
cd esp-idf-version
idf.py build flash monitor

# Для Arduino IDE
# Откройте bluetooth2brushless_hid_client.ino
```

## 📚 Документация

- 🎯 **[Какую версию выбрать?](WHICH_VERSION.md)** - Помощь в выборе подходящей версии
- 📖 **[Подробная инструкция HID Client](HID_CLIENT_README.md)** - Полное руководство по новой версии
- 🔧 **[Установка ESP-IDF](ESP_IDF_SETUP.md)** - Пошаговая настройка ESP-IDF
- ❓ **[FAQ](FAQ.md)** - Часто задаваемые вопросы
- 📋 **[Примеры](EXAMPLES.md)** - Примеры модификаций

## 🧪 Тестирование

```bash
# Тест подключения к BT13 в Ubuntu
python3 test_bt13_connection.py
```

## 🔗 Схема подключения

```
ESP32 GPIO 25 → Контроллер SPEED (PWM)
ESP32 GPIO 26 → Контроллер DIRECTION  
ESP32 GND     → Контроллер GND
ESP32 GPIO 2  → LED индикатор
```

## 🎮 Управление

| Кнопка BT13 | Функция |
|-------------|---------|
| Volume Up | Увеличить скорость |
| Volume Down | Уменьшить скорость |
| Play/Pause | Изменить направление |

## 📊 Статус проекта

- ✅ **ESP-IDF версия** - Полностью работает с BT13
- ⚠️ **Arduino IDE версия** - Базовая функциональность  
- ❌ **Оригинальная версия** - Не совместима с BT13

**Версия**: 1.1.0 | **Дата**: 2025-06-06 | **Статус**: Готов к использованию
