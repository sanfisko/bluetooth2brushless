# 🐧 Команды Ubuntu для анализа пульта BT13

## 🔍 Базовая диагностика

### Проверка Bluetooth адаптера
```bash
# Проверка наличия Bluetooth адаптера
lsusb | grep -i bluetooth
hciconfig -a

# Статус службы Bluetooth
sudo systemctl status bluetooth

# Включение Bluetooth (если выключен)
sudo systemctl start bluetooth
sudo hciconfig hci0 up
```

### Поиск устройств
```bash
# Поиск всех Bluetooth устройств
sudo hcitool scan

# Расширенный поиск с именами
sudo bluetoothctl
> scan on
> devices
> quit
```

## 🎯 Анализ BT13 (MAC: 8B:EB:75:4E:65:97)

### Получение информации об устройстве
```bash
# Подробная информация о BT13
sudo bluetoothctl info 8B:EB:75:4E:65:97

# Проверка активных соединений
hcitool con

# Проверка качества сигнала
hcitool rssi 8B:EB:75:4E:65:97
```

### Сопряжение с BT13
```bash
# Автоматическое сопряжение
sudo bluetoothctl
> agent on
> default-agent
> power on
> scan on
> pair 8B:EB:75:4E:65:97
> trust 8B:EB:75:4E:65:97
> connect 8B:EB:75:4E:65:97
> quit
```

## 🎮 HID анализ

### Проверка HID устройств
```bash
# Список HID устройств в системе
ls -la /dev/input/
ls -la /dev/input/by-id/

# Поиск Bluetooth HID устройств
ls /dev/input/by-id/ | grep -i bt

# Информация о input устройствах
cat /proc/bus/input/devices | grep -A 10 -B 5 -i bluetooth
```

### Мониторинг HID событий
```bash
# Установка evtest (если нет)
sudo apt install evtest

# Мониторинг событий от BT13
sudo evtest

# Альтернативный способ мониторинга
sudo cat /dev/input/event* | hexdump -C
```

## 🔧 Протоколы и сервисы

### Анализ SDP (Service Discovery Protocol)
```bash
# Поиск сервисов на BT13
sdptool browse 8B:EB:75:4E:65:97

# Поиск HID сервисов
sdptool search HID 8B:EB:75:4E:65:97

# Поиск всех сервисов
sdptool records 8B:EB:75:4E:65:97
```

### L2CAP анализ
```bash
# Проверка L2CAP соединений
sudo l2ping 8B:EB:75:4E:65:97

# Информация о L2CAP каналах
cat /proc/net/bluetooth/l2cap
```

## 📊 Логи и отладка

### Системные логи
```bash
# Bluetooth логи в dmesg
dmesg | grep -i bluetooth | tail -20

# Системные логи
sudo journalctl -u bluetooth -f

# Логи ядра
sudo tail -f /var/log/kern.log | grep -i bluetooth
```

### Включение отладки Bluetooth
```bash
# Включение отладочных логов
echo 'module bluetooth +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
echo 'module btusb +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
echo 'module hid +p' | sudo tee /sys/kernel/debug/dynamic_debug/control

# Просмотр отладочных логов
sudo dmesg -w | grep -i bluetooth
```

## 🐍 Python скрипт для анализа

```bash
# Запуск тестового скрипта
sudo python3 test_bt13_ubuntu.py

# Установка зависимостей (если нужно)
sudo apt install python3-bluetooth
pip3 install pybluez
```

## 🔍 Определение возможностей BT13

### Проверка HID дескрипторов
```bash
# После подключения BT13 как HID устройства
sudo usbhid-dump -d 8B:EB:75:4E:65:97

# Альтернативный способ
sudo hidraw-dump /dev/hidraw*
```

### Анализ кодов клавиш
```bash
# Мониторинг нажатий кнопок
sudo evtest /dev/input/eventX  # где X - номер BT13 устройства

# Показать все input устройства
sudo evtest
```

## 🚀 Быстрая проверка

```bash
# Одна команда для быстрой проверки
sudo hcitool scan | grep -i 8B:EB:75:4E:65:97 && echo "BT13 найден!" || echo "BT13 не найден"

# Проверка подключения
hcitool con | grep 8B:EB:75:4E:65:97 && echo "BT13 подключен!" || echo "BT13 не подключен"
```

## 💡 Полезные советы

1. **Если BT13 не виден**: Убедитесь что он в режиме сопряжения
2. **Если сопряжение не работает**: Попробуйте сбросить Bluetooth стек
3. **Для мониторинга**: Используйте `bluetoothctl` в интерактивном режиме
4. **Для отладки**: Включите verbose логи Bluetooth

## 🔄 Сброс Bluetooth (если что-то пошло не так)

```bash
# Перезапуск Bluetooth службы
sudo systemctl restart bluetooth

# Сброс Bluetooth адаптера
sudo hciconfig hci0 down
sudo hciconfig hci0 up

# Удаление сопряженных устройств
sudo bluetoothctl
> remove 8B:EB:75:4E:65:97
> quit
```