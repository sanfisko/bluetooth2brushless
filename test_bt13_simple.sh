#!/bin/bash
# Простой скрипт для тестирования BT13 без Python зависимостей
# Использует только стандартные утилиты Ubuntu

BT13_MAC="8B:EB:75:4E:65:97"

echo "🚀 Тестирование подключения к пульту BT13"
echo "=" * 50

# Проверка Bluetooth службы
echo "🔍 Проверка Bluetooth службы..."
if systemctl is-active --quiet bluetooth; then
    echo "✅ Bluetooth служба активна"
else
    echo "❌ Bluetooth служба неактивна"
    echo "Попробуйте: sudo systemctl start bluetooth"
    exit 1
fi

echo ""

# Проверка Bluetooth адаптера
echo "📡 Проверка Bluetooth адаптера..."
if command -v hciconfig &> /dev/null; then
    hciconfig -a | head -10
else
    echo "❌ hciconfig не найден. Установите: sudo apt install bluez"
    exit 1
fi

echo ""

# Поиск BT13
echo "🔍 Поиск пульта BT13 ($BT13_MAC)..."
echo "⏳ Сканирование 10 секунд..."

if command -v hcitool &> /dev/null; then
    scan_result=$(timeout 10 hcitool scan)
    echo "$scan_result"
    
    if echo "$scan_result" | grep -i "$BT13_MAC" > /dev/null; then
        echo "🎯 BT13 найден!"
        bt13_found=true
    else
        echo "❌ BT13 ($BT13_MAC) не найден"
        bt13_found=false
    fi
else
    echo "❌ hcitool не найден. Установите: sudo apt install bluez-tools"
    exit 1
fi

echo ""

if [ "$bt13_found" = true ]; then
    # Получение информации о BT13
    echo "📋 Получение информации о BT13..."
    if command -v bluetoothctl &> /dev/null; then
        echo "info $BT13_MAC" | bluetoothctl
    fi
    
    echo ""
    
    # Проверка сервисов
    echo "🔧 Проверка сервисов BT13..."
    if command -v sdptool &> /dev/null; then
        sdptool browse "$BT13_MAC" 2>/dev/null || echo "Не удалось получить сервисы (возможно нужно сопряжение)"
    else
        echo "sdptool не найден. Установите: sudo apt install bluez-tools"
    fi
    
    echo ""
    
    # Проверка активных соединений
    echo "🔗 Проверка активных соединений..."
    if hcitool con | grep -i "$BT13_MAC" > /dev/null; then
        echo "✅ BT13 подключен!"
        hcitool con | grep -i "$BT13_MAC"
    else
        echo "❌ BT13 не подключен"
    fi
    
else
    echo "💡 Попробуйте сопряжение с BT13:"
    echo "sudo bluetoothctl"
    echo "> power on"
    echo "> agent on"
    echo "> default-agent"
    echo "> scan on"
    echo "> pair $BT13_MAC"
    echo "> trust $BT13_MAC"
    echo "> connect $BT13_MAC"
    echo "> quit"
fi

echo ""

# Проверка HID устройств
echo "🎮 Проверка HID устройств..."
if ls /dev/input/by-id/ 2>/dev/null | grep -i bt; then
    echo "✅ Найдены Bluetooth HID устройства"
else
    echo "❌ Bluetooth HID устройства не найдены"
fi

echo ""

# Полезные команды
echo "📝 Полезные команды для дальнейшего анализа:"
echo "   bluetoothctl info $BT13_MAC"
echo "   hcitool con"
echo "   dmesg | grep -i bluetooth | tail -10"
echo "   sudo evtest  # для мониторинга HID событий"

echo ""
echo "🏁 Тестирование завершено"