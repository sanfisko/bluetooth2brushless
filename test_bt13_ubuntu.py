#!/usr/bin/env python3
"""
Тестовый скрипт для проверки подключения к пульту BT13 из Ubuntu
Помогает определить протоколы и возможности пульта

Использование:
sudo python3 test_bt13_ubuntu.py

Автор: OpenHands
Дата: 2025-06-06
"""

import bluetooth
import subprocess
import sys
import time
import os

BT13_MAC = "8B:EB:75:4E:65:97"

def check_bluetooth_service():
    """Проверка состояния Bluetooth службы"""
    try:
        result = subprocess.run(['systemctl', 'is-active', 'bluetooth'], 
                              capture_output=True, text=True)
        if result.stdout.strip() == 'active':
            print("✅ Bluetooth служба активна")
            return True
        else:
            print("❌ Bluetooth служба неактивна")
            print("Попробуйте: sudo systemctl start bluetooth")
            return False
    except Exception as e:
        print(f"❌ Ошибка проверки Bluetooth службы: {e}")
        return False

def scan_for_bt13():
    """Поиск пульта BT13"""
    print(f"🔍 Поиск пульта BT13 ({BT13_MAC})...")
    
    try:
        devices = bluetooth.discover_devices(duration=10, lookup_names=True)
        
        print(f"📱 Найдено устройств: {len(devices)}")
        
        bt13_found = False
        for addr, name in devices:
            print(f"  📍 {addr} - {name}")
            if addr.upper() == BT13_MAC.upper():
                bt13_found = True
                print(f"🎯 BT13 найден! Имя: {name}")
        
        if not bt13_found:
            print(f"❌ BT13 ({BT13_MAC}) не найден")
            print("💡 Убедитесь что:")
            print("   - BT13 включен")
            print("   - BT13 в режиме сопряжения")
            print("   - Bluetooth на компьютере включен")
        
        return bt13_found
        
    except Exception as e:
        print(f"❌ Ошибка поиска: {e}")
        return False

def get_device_info():
    """Получение информации об устройстве BT13"""
    print(f"📋 Получение информации о BT13...")
    
    try:
        # Используем bluetoothctl для получения подробной информации
        cmd = f"bluetoothctl info {BT13_MAC}"
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        
        if result.returncode == 0:
            print("📊 Информация о BT13:")
            print(result.stdout)
        else:
            print("❌ Не удалось получить информацию о BT13")
            print("💡 Попробуйте сначала выполнить сопряжение:")
            print(f"   bluetoothctl pair {BT13_MAC}")
            
    except Exception as e:
        print(f"❌ Ошибка получения информации: {e}")

def test_services():
    """Тестирование доступных сервисов"""
    print(f"🔧 Проверка сервисов BT13...")
    
    try:
        services = bluetooth.find_service(address=BT13_MAC)
        
        if services:
            print(f"🎛️ Найдено сервисов: {len(services)}")
            for service in services:
                print(f"  📡 Сервис: {service['name']}")
                print(f"     Протокол: {service['protocol']}")
                print(f"     Порт: {service['port']}")
                print(f"     Описание: {service.get('description', 'N/A')}")
                print()
        else:
            print("❌ Сервисы не найдены")
            print("💡 Возможно нужно выполнить сопряжение сначала")
            
    except Exception as e:
        print(f"❌ Ошибка проверки сервисов: {e}")

def test_hid_connection():
    """Тестирование HID подключения"""
    print(f"🎮 Тестирование HID подключения...")
    
    try:
        # Проверяем HID устройства в системе
        cmd = "ls /dev/input/by-id/ | grep -i bt"
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        
        if result.stdout:
            print("🎯 Найдены HID устройства:")
            print(result.stdout)
        else:
            print("❌ HID устройства не найдены")
            
        # Проверяем через hcitool
        cmd = f"hcitool con | grep {BT13_MAC}"
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        
        if result.stdout:
            print("🔗 Активное соединение найдено:")
            print(result.stdout)
        else:
            print("❌ Активное соединение не найдено")
            
    except Exception as e:
        print(f"❌ Ошибка тестирования HID: {e}")

def pair_bt13():
    """Попытка автоматического сопряжения с BT13"""
    print(f"🤝 Попытка сопряжения с BT13...")
    
    try:
        # Команды для bluetoothctl
        commands = [
            "power on",
            "agent on",
            "default-agent",
            f"pair {BT13_MAC}",
            f"trust {BT13_MAC}",
            f"connect {BT13_MAC}"
        ]
        
        for cmd in commands:
            print(f"  ⚡ Выполняю: {cmd}")
            full_cmd = f"echo '{cmd}' | bluetoothctl"
            result = subprocess.run(full_cmd, shell=True, capture_output=True, text=True)
            time.sleep(2)
            
        print("✅ Команды сопряжения выполнены")
        print("💡 Проверьте вывод выше на наличие ошибок")
        
    except Exception as e:
        print(f"❌ Ошибка сопряжения: {e}")

def main():
    print("🚀 Тестирование подключения к пульту BT13")
    print("=" * 50)
    
    # Проверка прав root
    if os.geteuid() != 0:
        print("❌ Запустите скрипт с правами root:")
        print("sudo python3 test_bt13_ubuntu.py")
        sys.exit(1)
    
    # Проверка Bluetooth службы
    if not check_bluetooth_service():
        sys.exit(1)
    
    print()
    
    # Поиск BT13
    bt13_found = scan_for_bt13()
    print()
    
    if bt13_found:
        # Получение информации
        get_device_info()
        print()
        
        # Проверка сервисов
        test_services()
        print()
        
        # Тестирование HID
        test_hid_connection()
        print()
    else:
        # Попытка сопряжения
        pair_bt13()
        print()
    
    print("🏁 Тестирование завершено")
    print()
    print("📝 Полезные команды для дальнейшего анализа:")
    print(f"   bluetoothctl info {BT13_MAC}")
    print(f"   hcitool con")
    print(f"   dmesg | grep -i bluetooth")
    print(f"   lsusb | grep -i bluetooth")

if __name__ == "__main__":
    main()