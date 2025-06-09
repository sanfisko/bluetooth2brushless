# ESP-IDF версия

## 🚀 Автоматическая установка

### Из любого места (рекомендуется)
```bash
wget https://raw.githubusercontent.com/sanfisko/bluetooth2brushless/main/install-esp-idf-version.sh
chmod +x install-esp-idf-version.sh
./install-esp-idf-version.sh
```

### Из папки проекта
```bash
cd esp-idf-version
./install.sh
```

Скрипт автоматически:
- ✅ Клонирует репозиторий (если нужно)
- ✅ Проверит ESP-IDF окружение
- ✅ Соберет проект
- ✅ Найдет ESP32
- ✅ Прошьет ESP32
- ✅ Запустит мониторинг

## 📋 Требования

### ESP-IDF v5.4+
```bash
mkdir -p ~/esp && cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32 && . ./export.sh
```

### Подключение ESP32
- USB кабель
- Порт: `/dev/ttyUSB0` или `/dev/ttyACM0`

## 🔧 Ручная установка

### 1. Активация ESP-IDF
```bash
. ~/esp/esp-idf/export.sh
```

### 2. Сборка
```bash
idf.py build
```

### 3. Прошивка
```bash
idf.py -p /dev/ttyUSB0 flash
```

### 4. Мониторинг
```bash
idf.py -p /dev/ttyUSB0 monitor
```

## 🎮 Управление BT13

| Кнопка | Действие |
|--------|----------|
| **+ короткое** | +1 уровень |
| **- короткое** | -1 уровень |
| **+ длинное** | Максимум вперед |
| **- длинное** | Максимум назад |
| **Средняя** | Стоп |

## 🔌 Подключение

```
ESP32 GPIO 25 → ESC PWM вход
ESP32 GPIO 26 → ESC реверс
ESP32 GND     → ESC GND
```

## 📊 Ожидаемые логи

```
BT13 подключен успешно!
Готов к приему команд от пульта
HID данные (3 байт): 01 B5 00
HID Usage: 0x00B5
Команда: Короткое + (увеличение уровня)
Состояние: ВКЛ | Уровень: 1/10 | PWM: 25/255
```

**MAC BT13**: `8B:EB:75:4E:65:97`