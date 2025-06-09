# Установка и настройка ESP-IDF v5.4.1 для проекта BT13

## Требования

- **ESP-IDF версия**: v5.4.1 (обязательно)
- **Целевая платформа**: ESP32
- **Операционная система**: Ubuntu/Debian, macOS
- **Python**: 3.8 или выше

## Шаг 0: Получение проекта

Сначала клонируйте проект bluetooth2brushless:

```bash
# Клонирование проекта
git clone https://github.com/sanfisko/bluetooth2brushless.git

# Переход в папку проекта
cd bluetooth2brushless

# Проверка содержимого
ls -la

# Должны увидеть папку esp-idf-version
ls esp-idf-version/
```

## Быстрый старт (если ESP-IDF уже установлен)

Если у вас уже установлен ESP-IDF v5.4.1 в `~/esp/esp-idf`, выполните:

```bash
# Активация окружения ESP-IDF
. ~/esp/esp-idf/export.sh

# Переход в папку проекта
cd bluetooth2brushless/esp-idf-version

# Проверка версии
idf.py --version

# Настройка целевой платформы
idf.py set-target esp32

# Сборка проекта
idf.py build
```

## Полная установка ESP-IDF v5.4.1

### Требования

- Python 3.8 или новее
- Git
- CMake 3.16 или новее

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
# Создание папки esp в домашней директории
mkdir -p ~/esp
cd ~/esp

# Клонирование ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git

# Переход в папку ESP-IDF
cd esp-idf

# Переключение на версию v5.4.1
git checkout v5.4.1
git submodule update --init --recursive
```

### 3. Установка инструментов

```bash
# Установка инструментов для ESP32
./install.sh esp32
```

### 4. Активация окружения

**Важно:** Команду активации нужно выполнять в каждой новой сессии терминала:

```bash
# Активация окружения ESP-IDF
. ~/esp/esp-idf/export.sh
```

### 5. Проверка установки

```bash
# Проверить версию
idf.py --version

# Должно показать:
# ESP-IDF v5.4.1
```

## Сборка проекта BT13

### 1. Подготовка проекта

```bash
# Переход в папку проекта
# Если клонировали в домашнюю папку:
cd ~/bluetooth2brushless/esp-idf-version
# Если клонировали в текущую папку:
# cd bluetooth2brushless/esp-idf-version

# Активация окружения ESP-IDF
. ~/esp/esp-idf/export.sh

# Настройка целевой платформы
idf.py set-target esp32
```

### 2. Конфигурация проекта

```bash
idf.py menuconfig
```

#### Важные настройки в menuconfig:

```
Component config → Bluetooth →
  [*] Bluetooth
  [*] Classic Bluetooth  
  [*] HID Host
  [ ] BLE (отключить для экономии памяти)

Component config → Bluetooth → Controller Options →
  Controller mode → Bluetooth Dual Mode (BTDM)
  (НЕ выбирайте "BLE only"!)

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

# Прошивка (подключите ESP32 к компьютеру)
idf.py -p /dev/ttyUSB0 flash

# Мониторинг вывода
idf.py -p /dev/ttyUSB0 monitor
```

**Примечание:** Замените `/dev/ttyUSB0` на правильный порт:
- Linux: `/dev/ttyUSB0` или `/dev/ttyACM0`
- macOS: `/dev/cu.usbserial-*` или `/dev/cu.SLAB_USBtoUART`
- Windows: `COM3`, `COM4` и т.д.

## Решение проблем

### Команда "idf.py: команда не найдена"

Это означает, что окружение ESP-IDF не активировано. Выполните:

```bash
. ~/esp/esp-idf/export.sh
```

### Проблемы с портом

1. **Проверьте подключение ESP32**
2. **Найдите правильный порт:**
   ```bash
   # Linux/macOS
   ls /dev/tty*
   
   # Или используйте автоопределение
   idf.py flash
   ```
3. **Проверьте драйверы USB-UART:**
   - CP2102: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
   - CH340: http://www.wch.cn/downloads/CH341SER_EXE.html
3. **Нажмите кнопку BOOT** на ESP32 при прошивке

### Ошибки компиляции

#### Ошибка "esp_hidd_prf_api.h: No such file or directory"

Эта ошибка возникает в ESP-IDF v5.4+ из-за изменений в API. Исправление:

```c
// Удалить эту строку из main.c:
// #include "esp_hidd_prf_api.h"

// Заменить на правильный заголовочный файл для ESP-IDF v5.4+:
#include "esp_hidh_api.h"
```

#### Ошибка "esp_hid_gap.h: No such file or directory"

В ESP-IDF v5.4+ API HID изменился. Используйте:

```c
// Вместо:
// #include "esp_hid_gap.h"
// #include "esp_hid_host.h"

// Используйте:
#include "esp_hidh_api.h"
```

#### Предупреждение "unknown kconfig symbol 'LEDC_USE_XTAL_CLK'"

В `sdkconfig.defaults` закомментируйте устаревшую опцию:

```bash
# CONFIG_LEDC_USE_XTAL_CLK=y
```

#### Ошибка "CONFIG_BTDM_CTRL_MODE_BLE_ONLY=y" (неправильная конфигурация Bluetooth)

Если в `sdkconfig` установлено `CONFIG_BTDM_CTRL_MODE_BLE_ONLY=y`, это неправильно для HID Host. Исправление:

```bash
# Удалить существующий sdkconfig
rm sdkconfig

# Пересоздать конфигурацию
idf.py set-target esp32

# Или исправить в menuconfig:
idf.py menuconfig
# Перейти в: Component config → Bluetooth → Controller Options
# Выбрать: Controller mode → Bluetooth Dual Mode (BTDM)
```

#### Общие ошибки компиляции

```bash
# Очистить сборку и конфигурацию
idf.py fullclean
rm sdkconfig

# Пересоздать конфигурацию и собрать
idf.py set-target esp32
idf.py build
```

#### Если HID заголовочные файлы не найдены

Убедитесь, что в `sdkconfig.defaults` есть:

```bash
CONFIG_BT_HID_HOST_ENABLED=y
CONFIG_BT_HID_ENABLED=y
CONFIG_ESP_HID_HOST_ENABLED=y
```

### Проблемы с Bluetooth

1. **Проверьте конфигурацию** в menuconfig

2. **Убедитесь, что BT13 в режиме сопряжения:**
   - Нажмите и удерживайте кнопку питания BT13
   - Светодиод должен мигать синим

3. **Проверьте логи:**
   ```bash
   idf.py monitor
   ```

4. **Сброс настроек Bluetooth на ESP32:**
   ```bash
   idf.py erase-flash
   idf.py flash
   ```

## Дополнительные ресурсы

- [Официальная документация ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/)
- [ESP32 Bluetooth Classic API](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/bluetooth/classic_bt.html)
- [ESP32 HID Host API](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/bluetooth/esp_hidh.html)

---

**Примечание**: Этот проект протестирован и работает с ESP-IDF v5.4.1. Использование других версий может привести к ошибкам компиляции.