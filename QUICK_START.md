# 🚀 Быстрый старт bluetooth2brushless

## 🎯 Выбери свой путь

### 🟢 Новичкам → Arduino IDE
```bash
# 1. Скачай Arduino IDE 2.x
# 2. Добавь ESP32: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
# 3. Открой arduino-ide-version/bluetooth2brushless_hid_client.ino
# 4. Загрузи в ESP32
```

### 🔵 Продвинутым → ESP-IDF (автоустановка)
```bash
wget https://raw.githubusercontent.com/sanfisko/bluetooth2brushless/main/install-esp-idf-version.sh
chmod +x install-esp-idf-version.sh
./install-esp-idf-version.sh
```

## 🔌 Подключение

```
ESP32 GPIO 25 → ESC PWM вход
ESP32 GPIO 26 → ESC реверс  
ESP32 GND     → ESC GND
```

**⚠️ Используй драйвер мотора (ESC)!**

## 🎮 Управление BT13

| Кнопка | Действие |
|--------|----------|
| **+ короткое** | +1 уровень скорости |
| **- короткое** | -1 уровень скорости |
| **+ длинное** | Максимум вперед |
| **- длинное** | Максимум назад |
| **Средняя** | Полная остановка |

## 📋 Что нужно

- ESP32 (любой)
- Пульт BT13 (MAC: 8B:EB:75:4E:65:97)
- ESC (драйвер мотора)
- Бесщеточный мотор

## ✅ Ожидаемый результат

```
BT13 подключен успешно!
Готов к приему команд от пульта
HID Usage: 0x00B5
Команда: Короткое + (увеличение уровня)
Состояние: ВКЛ | Уровень: 1/10 | PWM: 25/255
```

## 🆘 Не работает?

1. **Проверь провода** (90% проблем)
2. **Зарядил BT13?** 
3. **Правильный порт ESP32?**
4. **Используешь ESC?**

## 📚 Подробнее

- [INSTALLATION.md](INSTALLATION.md) - Детальная установка
- [WIRING.md](WIRING.md) - Схема подключения
- [BT13_HID_ANALYSIS.md](BT13_HID_ANALYSIS.md) - Анализ протокола

**Готово! Управляй мотором с пульта BT13! 🎉**