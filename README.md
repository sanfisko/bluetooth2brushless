# 🚗 bluetooth2brushless

Управляй мотором с пульта BT13. Просто работает™

## 🤔 Что это?

ESP32 + пульт BT13 + мотор = магия управления на расстоянии

**Проблема была**: ESP32 не дружил с BT13  
**Проблема решена**: Теперь дружат как лучшие друзья

## 🔍 Последние обновления

**✅ ПРОТЕСТИРОВАНО НА РЕАЛЬНОМ BT13!**
- Определены точные HID коды всех кнопок
- BT13 сам различает короткие/длинные нажатия
- Код максимально упрощен и оптимизирован

## 🚀 Быстрый старт (для ленивых)

### Вариант 1: Arduino IDE (5 минут)
```bash
1. Открой bluetooth2brushless_hid_client.ino
2. Нажми "Загрузить" 
3. Включи BT13
4. Profit! 🎉
```

### Вариант 2: ESP-IDF (для крутых)
```bash
cd esp-idf-version
idf.py build flash monitor
# Смотри как ESP32 сам находит BT13
```

## 🔌 Подключение (не перепутай!)

```
ESP32 GPIO 25 → Мотор SPEED
ESP32 GPIO 26 → Мотор DIRECTION  
ESP32 GND     → Мотор GND
```

## 🎮 Кнопки пульта (ОБНОВЛЕНО!)

| Нажатие | HID код | Что делает |
|---------|---------|------------|
| 🔊+ короткое | `NEXTSONG` | +1 уровень (10 нажатий до максимума) |
| 🔉- короткое | `PREVIOUSSONG` | -1 уровень (через 0 меняется направление) |
| 🔊+ длинное | `VOLUMEUP` | Максимум вперед |
| 🔉- длинное | `VOLUMEDOWN` | Максимум назад |
| ⏯️ | `PLAYPAUSE` | СТОП из любого режима |

**💡 Секрет**: BT13 сам определяет длинные нажатия и отправляет разные коды!

## 📁 Какой файл брать?

- **Новичок**: `bluetooth2brushless_hid_client.ino`
- **Профи**: `esp-idf-version/`
- **Мазохист**: `bluetooth2brushless.ino` (не работает с BT13)

## 🧪 Тестирование BT13

### Ubuntu/Linux (рекомендуется)
```bash
# Быстрый тест без зависимостей
sudo ./test_bt13_simple.sh

# Полный тест (нужен python3-bluetooth)
sudo apt install python3-bluetooth
sudo python3 test_bt13_ubuntu.py

# Мониторинг кнопок в реальном времени
sudo evtest /dev/input/event11
```

## 🆘 Не работает?

1. **Проверь провода** (90% проблем)
2. **Зарядил BT13?** (включается ли?)
3. **MAC правильный?** `8B:EB:75:4E:65:97`
4. **Протестируй в Ubuntu** (см. выше)
5. Читай документацию ниже
6. Плачь в Issues

## 📚 Документация

### Основная
- [INSTALLATION.md](INSTALLATION.md) - Установка Arduino IDE
- [HID_CLIENT_README.md](HID_CLIENT_README.md) - Техническая документация
- [QUICK_TEST.md](QUICK_TEST.md) - Быстрое тестирование

### Анализ BT13
- [BT13_HID_ANALYSIS.md](BT13_HID_ANALYSIS.md) - **Полный анализ протокола**
- [ubuntu_bt13_commands.md](ubuntu_bt13_commands.md) - Команды Ubuntu
- [test_bt13_ubuntu.py](test_bt13_ubuntu.py) - Python тестирование
- [test_bt13_simple.sh](test_bt13_simple.sh) - Bash тестирование

### История изменений
- [MOTOR_CONTROL_UPDATE.md](MOTOR_CONTROL_UPDATE.md) - Логика управления

**Версия**: 2.0.0 | **Статус**: Протестировано на реальном BT13! ✅
