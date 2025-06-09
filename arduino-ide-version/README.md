# Arduino IDE версия

## 🚀 Быстрый старт

1. Открой `bluetooth2brushless_hid_client.ino` в Arduino IDE
2. Выбери плату: ESP32 Dev Module
3. Выбери порт ESP32
4. Нажми "Загрузить"

## 📁 Файлы

- `bluetooth2brushless_hid_client.ino` - основной код
- `arduino_config.txt` - настройки Arduino IDE

## 🔌 Подключение

```
ESP32 GPIO 25 → ESC PWM вход
ESP32 GPIO 26 → ESC реверс
ESP32 GND     → ESC GND
```

## 🎮 Управление BT13

- **+ короткое** → +1 уровень
- **- короткое** → -1 уровень  
- **+ длинное** → максимум вперед
- **- длинное** → максимум назад
- **Средняя** → стоп

**MAC BT13**: `8B:EB:75:4E:65:97`