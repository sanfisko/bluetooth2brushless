# 🚗 bluetooth2brushless

ESP32 + пульт BT13 = управление мотором

## 🚀 Быстрый старт

### Arduino IDE (рекомендуется)
1. Открой `arduino-ide-version/bluetooth2brushless_hid_client.ino`
2. Загрузи в ESP32
3. Включи BT13 → автоподключение

### ESP-IDF (продвинутые)
```bash
cd esp-idf-version
idf.py build flash monitor
```

## 🔌 Подключение

```
ESP32 GPIO 25 → Мотор SPEED (PWM)
ESP32 GPIO 26 → Мотор DIRECTION
ESP32 GND     → Мотор GND
```

**⚠️ Используй драйвер мотора (ESC)!** Подробно: [WIRING.md](WIRING.md)

## 🎮 Управление

| Кнопка BT13 | Действие |
|-------------|----------|
| **+ короткое** | +1 уровень скорости |
| **- короткое** | -1 уровень скорости |
| **+ длинное** | Максимум вперед |
| **- длинное** | Максимум назад |
| **Средняя** | СТОП |

## 📁 Структура

- `arduino-ide-version/` - Версия для Arduino IDE
- `esp-idf-version/` - Версия для ESP-IDF
- `WIRING.md` - Схема подключения
- `INSTALLATION.md` - Установка

## 🧪 Тестирование BT13

```bash
# Ubuntu/Linux
sudo ./test_bt13_simple.sh
```

**MAC BT13**: `8B:EB:75:4E:65:97`
