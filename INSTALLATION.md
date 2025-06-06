# 🔧 Установка (для чайников)

## 🛒 Что купить

- ESP32 (любой, главное чтоб работал)
- Контроллер мотора (с PWM входом)
- Мотор бесщеточный (не сожги дом)
- Пульт BT13 (именно BT13!)
- Провода (много проводов)

## 💻 Софт

### Arduino IDE (простой путь)
```bash
1. Скачай Arduino IDE 2.0+
2. Файл → Настройки → Дополнительные ссылки:
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
3. Инструменты → Менеджер плат → ESP32 → Установить
4. Готово!
```

### ESP-IDF (для мазохистов)
```bash
# Ubuntu/macOS
git clone https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32
. ./export.sh
# Теперь можешь страдать профессионально
```

## 🔌 Подключение

```
ESP32 GPIO 25 → Мотор SPEED    (не перепутай!)
ESP32 GPIO 26 → Мотор DIR      (направление)
ESP32 GND     → Мотор GND      (земля общая)

Мотор питание: 12-36V (не от ESP32!)
```

⚠️ **НЕ ПОДКЛЮЧАЙ МОТОР К 3.3V ESP32!** (сгорит нафиг)

## 📱 Загрузка кода

### Вариант 1: Arduino IDE
```bash
1. Открой bluetooth2brushless_hid_client.ino
2. Плата: ESP32 Dev Module
3. Порт: тот что появился когда воткнул ESP32
4. Нажми "Загрузить"
5. Жди и молись
```

### Вариант 2: ESP-IDF
```bash
cd esp-idf-version
idf.py build        # собрать
idf.py flash        # прошить
idf.py monitor      # смотреть логи
```

## 🎮 Тест

1. Включи BT13
2. ESP32 сам его найдет (если не дурак)
3. Жми кнопки на пульте
4. Мотор крутится = победа! 🎉  
- `P` - изменить направление
- `S` - остановить двигатель
- `E` - включить/выключить двигатель

### Через пульт BT13
1. Включите пульт BT13
2. Выполните сопряжение с ESP32
3. Используйте кнопки:
   - Vol+ (Увеличение громкости) - увеличение скорости
   - Vol- (Уменьшение громкости) - уменьшение скорости
   - Play/Pause - изменение направления

## Устранение неполадок

### ESP32 не определяется
- Проверьте USB-кабель (должен поддерживать передачу данных)
- Установите драйверы CH340/CP2102 для вашей ОС
- Попробуйте другой USB-порт

### Ошибки компиляции
- Убедитесь, что установлен пакет ESP32
- Проверьте выбор правильной платы
- Перезапустите Arduino IDE

### Bluetooth не подключается
- Проверьте, что ESP32 запущен и инициализирован
- Убедитесь, что Bluetooth включен на устройстве
- Попробуйте "забыть" устройство и подключиться заново

### Двигатель не реагирует
- Проверьте подключение проводов согласно схеме
- Убедитесь в правильности питания контроллера
- Проверьте общий GND между ESP32 и контроллером
- Измерьте напряжение на выходе PWM (должно изменяться от 0 до 3.3V)

## Дополнительные настройки

### Изменение параметров PWM
В файле `bluetooth2brushless.ino` можно изменить:
```cpp
const int freq = 1000;          // Частота PWM (Гц)
const int speedStep = 10;       // Шаг изменения скорости
const int maxSpeed = 255;       // Максимальная скорость
```

### Изменение имени Bluetooth
```cpp
SerialBT.begin("ESP32_Motor_Control");  // Замените на желаемое имя
```

### Настройка таймаута
```cpp
const unsigned long commandTimeout = 5000; // Таймаут в миллисекундах
```

## Безопасность

⚠️ **ВАЖНО**: 
- Всегда отключайте питание при подключении проводов
- Не превышайте номинальные характеристики двигателя и контроллера
- Обеспечьте надежное крепление двигателя
- Используйте предохранители в цепи питания
- Не оставляйте систему без присмотра при работе на высоких скоростях