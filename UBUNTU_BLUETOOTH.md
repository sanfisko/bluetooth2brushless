# 🐧 Bluetooth на Ubuntu 24.04 LTS

Краткое руководство по настройке Bluetooth для работы с install_bt.sh на Ubuntu 24.04.

## 🚀 Быстрая настройка

```bash
# 1. Обновление системы
sudo apt update && sudo apt upgrade -y

# 2. Установка Bluetooth пакетов
sudo apt install -y bluetooth bluez bluez-tools

# 3. Запуск и включение службы
sudo systemctl start bluetooth
sudo systemctl enable bluetooth

# 4. Проверка статуса
sudo systemctl status bluetooth
```

## 🔧 Диагностика проблем

### Проверка Bluetooth адаптера:
```bash
# Список Bluetooth устройств
lsusb | grep -i bluetooth
lspci | grep -i bluetooth

# Статус rfkill
rfkill list bluetooth

# Если заблокирован:
sudo rfkill unblock bluetooth
```

### Проверка службы:
```bash
# Статус службы
systemctl status bluetooth

# Перезапуск службы
sudo systemctl restart bluetooth

# Логи службы
journalctl -u bluetooth -f
```

### Ручное тестирование:
```bash
# Запуск bluetoothctl
bluetoothctl

# В bluetoothctl:
power on
agent on
default-agent
scan on
# Ждем 10-15 секунд
devices
scan off
quit
```

## 🎯 Для BT13 пульта

### Режимы работы BT13:
- **Красный + Синий мигание** = Режим поиска (готов к подключению)
- **Только синий мигание** = Подключен к устройству
- **Не мигает** = Выключен

### Включение режима поиска:
1. **Долгое нажатие** средней кнопки (3-5 секунд)
2. Должен начать мигать красным и синим
3. Если мигает только синим - отключите от других устройств

### Сброс подключений:
1. Выключите BT13
2. На телефоне/компьютере: "Забыть устройство BT13"
3. Включите BT13 снова

## 🛠️ Решение проблем

### Проблема: "No default controller available"
```bash
# Перезапуск Bluetooth стека
sudo systemctl stop bluetooth
sudo systemctl start bluetooth

# Или полная перезагрузка модуля
sudo modprobe -r btusb
sudo modprobe btusb
```

### Проблема: "Permission denied"
```bash
# Добавление пользователя в группу bluetooth
sudo usermod -a -G bluetooth $USER

# Перелогиниться или:
newgrp bluetooth
```

### Проблема: Сканирование не находит устройства
```bash
# 1. Очистка кэша Bluetooth
sudo rm -rf /var/lib/bluetooth/*
sudo systemctl restart bluetooth

# 2. Сброс настроек bluetoothctl
bluetoothctl
remove *
quit

# 3. Запуск с правами root
sudo ./install_bt.sh
```

## 📱 Альтернативные способы найти MAC

### Через GUI (если установлен):
1. Настройки → Bluetooth
2. Включить BT13 в режим поиска
3. Найти "BT13" в списке
4. Скопировать MAC адрес

### Через телефон:
1. Настройки → Bluetooth
2. Поиск устройств
3. Найти "BT13"
4. Посмотреть MAC в свойствах

### Через другие инструменты:
```bash
# nmap (если установлен)
sudo nmap -sn 192.168.1.0/24

# bluetoothctl в интерактивном режиме
bluetoothctl
scan on
# Ждем
devices
```

## ✅ Проверка готовности

Перед запуском install_bt.sh убедитесь:

```bash
# 1. Bluetooth служба работает
systemctl is-active bluetooth
# Должно вернуть: active

# 2. Адаптер не заблокирован
rfkill list bluetooth
# Soft blocked: no
# Hard blocked: no

# 3. bluetoothctl доступен
which bluetoothctl
# Должен показать путь

# 4. BT13 в режиме поиска
# Мигает красным и синим
```

Если все проверки пройдены - запускайте `./install_bt.sh`!

---

**💡 Совет**: Если ничего не помогает, попробуйте запустить скрипт с sudo или используйте ручной ввод MAC адреса в скрипте.