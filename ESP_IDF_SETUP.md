# Установка и настройка ESP-IDF для проекта BT13

## Быстрый старт (если ESP-IDF уже установлен)

Если у вас уже установлен ESP-IDF в `~/esp/esp-idf`, выполните:

```bash
# Активация окружения ESP-IDF
. ~/esp/esp-idf/export.sh

# Переход в папку проекта
cd ~/bluetooth2brushless/esp-idf-version

# Проверка версии
idf.py --version

# Настройка целевой платформы
idf.py set-target esp32

# Сборка проекта
idf.py build
```

## Полная установка ESP-IDF на Ubuntu/macOS

> **Примечание**: Инструкции обновлены для ESP-IDF v5.4.1 и корректного пути установки в `~/esp/esp-idf`

### 1. Установка зависимостей

#### Ubuntu/Debian:
```bash
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

#### macOS:
```bash
# Установите Homebrew если не установлен
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Установите зависимости
brew install cmake ninja dfu-util
```

### 2. Клонирование ESP-IDF

```bash
# Проверка существующей установки
if [ -d "$HOME/esp/esp-idf" ]; then
    echo "ESP-IDF уже установлен в ~/esp/esp-idf"
    cd ~/esp/esp-idf
    git pull
else
    # Создание папки esp в домашней директории
    mkdir -p ~/esp
    cd ~/esp
    
    # Клонирование ESP-IDF
    git clone --recursive https://github.com/espressif/esp-idf.git
    
    # Переход в папку ESP-IDF
    cd esp-idf
fi

# Переключение на стабильную версию (рекомендуется использовать последнюю стабильную)
git checkout v5.4.1
git submodule update --init --recursive
```

### 3. Установка инструментов

```bash
# Установка инструментов для ESP32
./install.sh esp32

# Настройка окружения (добавьте в ~/.bashrc или ~/.zshrc)
echo 'alias get_idf=". $HOME/esp/esp-idf/export.sh"' >> ~/.bashrc

# Применить изменения
source ~/.bashrc

# Или для текущей сессии
. ./export.sh
```

### 4. Проверка установки

```bash
# Активировать окружение ESP-IDF
get_idf

# Или если алиас не работает, используйте прямую команду:
# . ~/esp/esp-idf/export.sh

# Проверить версию
idf.py --version

# Должно показать что-то вроде:
# ESP-IDF v5.4.1
```

## Сборка проекта BT13

### 1. Подготовка проекта

```bash
# Переход в папку проекта
cd ~/bluetooth2brushless/esp-idf-version

# Активация окружения ESP-IDF
get_idf
# Или если алиас не работает:
# . ~/esp/esp-idf/export.sh

# Настройка целевой платформы
idf.py set-target esp32
```

### 2. Конфигурация проекта

```bash
# Открыть меню конфигурации
idf.py menuconfig
```

#### Важные настройки в menuconfig:

```
Component config → Bluetooth →
  [*] Bluetooth
  [*] Classic Bluetooth  
  [*] HID Host
  [ ] BLE (отключить для экономии памяти)

Component config → Bluetooth → Bluedroid Options →
  [*] Classic Bluetooth
  [*] HID Host

Component config → Log output →
  Default log verbosity → Info

Partition Table →
  Partition Table → Single factory app, no OTA
```

### 3. Сборка и прошивка

```bash
# Сборка проекта
idf.py build

# Подключите ESP32 к компьютеру и найдите порт
ls /dev/tty* | grep -E "(USB|ACM)"

# Прошивка (замените /dev/ttyUSB0 на ваш порт)
idf.py -p /dev/ttyUSB0 flash

# Мониторинг логов
idf.py -p /dev/ttyUSB0 monitor

# Выход из монитора: Ctrl+]
```

## Устранение проблем

### Ошибка "get_idf: команда не найдена"

Если алиас `get_idf` не работает, используйте прямую команду:

```bash
# Активация ESP-IDF окружения
. ~/esp/esp-idf/export.sh

# Или создайте алиас заново
echo 'alias get_idf=". $HOME/esp/esp-idf/export.sh"' >> ~/.bashrc
source ~/.bashrc

# Для zsh пользователей
echo 'alias get_idf=". $HOME/esp/esp-idf/export.sh"' >> ~/.zshrc
source ~/.zshrc
```

### Ошибка прав доступа к порту

```bash
# Добавить пользователя в группу dialout
sudo usermod -a -G dialout $USER

# Перелогиниться или выполнить
newgrp dialout

# Или дать права на порт
sudo chmod 666 /dev/ttyUSB0
```

### Ошибка "No module named 'serial'"

```bash
# Установить pyserial
pip3 install pyserial
```

### ESP32 не определяется

1. **Проверьте USB кабель** - должен поддерживать передачу данных
2. **Установите драйверы**:
   - CP2102: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
   - CH340: http://www.wch.cn/downloads/CH341SER_EXE.html
3. **Нажмите кнопку BOOT** на ESP32 при прошивке

### Ошибки компиляции

```bash
# Очистить сборку
idf.py fullclean

# Пересобрать
idf.py build
```

### Проблемы с Bluetooth

1. **Проверьте конфигурацию** в menuconfig
2. **Убедитесь что BT13 не подключен** к другим устройствам
3. **Перезагрузите ESP32** после прошивки

## Полезные команды

```bash
# Очистка проекта
idf.py clean

# Полная очистка
idf.py fullclean

# Только прошивка без сборки
idf.py -p /dev/ttyUSB0 flash

# Только мониторинг
idf.py -p /dev/ttyUSB0 monitor

# Прошивка и мониторинг
idf.py -p /dev/ttyUSB0 flash monitor

# Информация о разделах
idf.py partition-table

# Размер прошивки
idf.py size

# Анализ размера компонентов
idf.py size-components
```

## Структура проекта ESP-IDF

```
esp-idf-version/
├── CMakeLists.txt          # Основной файл сборки
├── sdkconfig.defaults      # Настройки по умолчанию
├── main/
│   ├── CMakeLists.txt      # Файл сборки main компонента
│   └── main.c              # Основной код
└── build/                  # Папка сборки (создается автоматически)
```

## Отладка

### Уровни логирования

```c
ESP_LOGE(TAG, "Ошибка: %s", error_msg);    // Красный
ESP_LOGW(TAG, "Предупреждение: %s", msg);  // Желтый  
ESP_LOGI(TAG, "Информация: %s", msg);      // Зеленый
ESP_LOGD(TAG, "Отладка: %s", msg);         // Белый
ESP_LOGV(TAG, "Подробно: %s", msg);        // Серый
```

### Изменение уровня логирования

```bash
# В menuconfig
Component config → Log output → Default log verbosity

# Или в коде
esp_log_level_set("BT13_MOTOR_CONTROL", ESP_LOG_DEBUG);
```

### Мониторинг с фильтрацией

```bash
# Показать только сообщения нашего компонента
idf.py monitor | grep "BT13_MOTOR_CONTROL"

# Показать только ошибки
idf.py monitor | grep -E "(ERROR|ERRO)"
```

## Переход с Arduino IDE на ESP-IDF

### Основные отличия:

| Arduino IDE | ESP-IDF |
|-------------|---------|
| `setup()` | `app_main()` |
| `loop()` | `while(1)` в `app_main()` |
| `delay()` | `vTaskDelay(pdMS_TO_TICKS())` |
| `Serial.println()` | `ESP_LOGI()` |
| `pinMode()` | `gpio_config()` |
| `digitalWrite()` | `gpio_set_level()` |
| `analogWrite()` | `ledc_set_duty()` |

### Преимущества ESP-IDF:
- Полный контроль над системой
- Лучшая производительность
- Профессиональные инструменты отладки
- Поддержка всех возможностей ESP32
- Стабильность в продакшене

### Недостатки ESP-IDF:
- Более сложная настройка
- Больше кода для простых задач
- Требует знания FreeRTOS
- Дольше время разработки