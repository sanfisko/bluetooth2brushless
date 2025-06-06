#!/usr/bin/env python3
"""
Тестовый скрипт для проверки подключения к BT13 пульту
Симулирует HID команды для тестирования ESP32

Автор: OpenHands
Дата: 2025-06-06
"""

import bluetooth
import time
import sys

# MAC адрес пульта BT13
BT13_MAC = "8B:EB:75:4E:65:97"

# HID коды кнопок (из анализа Ubuntu)
HID_CODES = {
    'VOLUME_UP': 0xE9,
    'VOLUME_DOWN': 0xEA, 
    'PLAY_PAUSE': 0xCD,
    'NEXT_SONG': 0xB5,
    'PREV_SONG': 0xB6
}

def scan_for_bt13():
    """Поиск пульта BT13"""
    print("🔍 Поиск Bluetooth устройств...")
    
    try:
        devices = bluetooth.discover_devices(duration=10, lookup_names=True)
        
        for addr, name in devices:
            print(f"Найдено: {name} ({addr})")
            if addr.upper() == BT13_MAC.upper():
                print(f"✅ Найден BT13: {name} ({addr})")
                return True
                
        print(f"❌ BT13 ({BT13_MAC}) не найден")
        return False
        
    except Exception as e:
        print(f"❌ Ошибка поиска: {e}")
        return False

def test_bt13_connection():
    """Тестирование подключения к BT13"""
    print(f"🔗 Попытка подключения к BT13 ({BT13_MAC})...")
    
    try:
        # Попытка подключения по HID протоколу
        sock = bluetooth.BluetoothSocket(bluetooth.L2CAP)
        sock.connect((BT13_MAC, 17))  # HID Control channel
        
        print("✅ Подключение к BT13 успешно!")
        sock.close()
        return True
        
    except Exception as e:
        print(f"❌ Ошибка подключения: {e}")
        return False

def simulate_hid_commands():
    """Симуляция HID команд для тестирования"""
    print("🎮 Симуляция команд пульта BT13...")
    
    commands = [
        ("Увеличение громкости", HID_CODES['VOLUME_UP']),
        ("Уменьшение громкости", HID_CODES['VOLUME_DOWN']),
        ("Воспроизведение/Пауза", HID_CODES['PLAY_PAUSE'])
    ]
    
    for name, code in commands:
        print(f"📤 {name} (0x{code:02X})")
        time.sleep(1)

def check_bluetooth_status():
    """Проверка статуса Bluetooth"""
    print("📡 Проверка статуса Bluetooth...")
    
    try:
        # Проверка доступности Bluetooth адаптера
        devices = bluetooth.discover_devices(duration=1)
        print("✅ Bluetooth адаптер работает")
        return True
    except Exception as e:
        print(f"❌ Проблема с Bluetooth: {e}")
        print("💡 Попробуйте:")
        print("   sudo systemctl restart bluetooth")
        print("   sudo hciconfig hci0 up")
        return False

def main():
    """Основная функция тестирования"""
    print("=" * 50)
    print("🧪 ТЕСТ ПОДКЛЮЧЕНИЯ К ПУЛЬТУ BT13")
    print("=" * 50)
    
    # 1. Проверка Bluetooth
    if not check_bluetooth_status():
        sys.exit(1)
    
    print()
    
    # 2. Поиск BT13
    if not scan_for_bt13():
        print("\n💡 Убедитесь что:")
        print("   - BT13 включен")
        print("   - BT13 в режиме сопряжения")
        print("   - BT13 не подключен к другим устройствам")
        sys.exit(1)
    
    print()
    
    # 3. Тестирование подключения
    test_bt13_connection()
    
    print()
    
    # 4. Симуляция команд
    simulate_hid_commands()
    
    print()
    print("=" * 50)
    print("✅ ТЕСТ ЗАВЕРШЕН")
    print("=" * 50)
    print("\n📋 Результаты для ESP32:")
    print(f"   MAC адрес BT13: {BT13_MAC}")
    print("   Коды кнопок:")
    for name, code in HID_CODES.items():
        print(f"     {name}: 0x{code:02X}")
    
    print("\n🔧 Следующие шаги:")
    print("   1. Прошейте ESP32 новым кодом")
    print("   2. Убедитесь что BT13 отключен от других устройств")
    print("   3. Запустите ESP32 и проверьте логи")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⏹️  Тест прерван пользователем")
    except Exception as e:
        print(f"\n❌ Неожиданная ошибка: {e}")
        sys.exit(1)