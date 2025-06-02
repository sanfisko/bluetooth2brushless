#!/usr/bin/env python3
"""
Утилита для тестирования ESP32 Motor Control через Bluetooth
Требует: pip install pybluez

Использование:
python test_commands.py
"""

import bluetooth
import time
import sys

class ESP32MotorTester:
    def __init__(self, device_name="ESP32_Motor_Control"):
        self.device_name = device_name
        self.socket = None
        self.connected = False
        
    def scan_devices(self):
        """Поиск Bluetooth устройств"""
        print("Поиск Bluetooth устройств...")
        devices = bluetooth.discover_devices(duration=8, lookup_names=True)
        
        print(f"Найдено {len(devices)} устройств:")
        for addr, name in devices:
            print(f"  {name} - {addr}")
            if name == self.device_name:
                return addr
        return None
    
    def connect(self, address=None):
        """Подключение к ESP32"""
        if not address:
            address = self.scan_devices()
            if not address:
                print(f"Устройство {self.device_name} не найдено!")
                return False
        
        try:
            print(f"Подключение к {address}...")
            self.socket = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
            self.socket.connect((address, 1))  # Канал 1 для SPP
            self.connected = True
            print("Подключение установлено!")
            return True
        except Exception as e:
            print(f"Ошибка подключения: {e}")
            return False
    
    def disconnect(self):
        """Отключение"""
        if self.socket:
            self.socket.close()
            self.connected = False
            print("Отключено")
    
    def send_command(self, command):
        """Отправка команды"""
        if not self.connected:
            print("Не подключено к устройству!")
            return False
        
        try:
            self.socket.send(command.encode())
            print(f"Отправлено: {command}")
            
            # Ожидание ответа
            time.sleep(0.1)
            try:
                response = self.socket.recv(1024).decode()
                if response:
                    print(f"Ответ: {response.strip()}")
            except:
                pass  # Таймаут - нормально
            
            return True
        except Exception as e:
            print(f"Ошибка отправки: {e}")
            return False
    
    def interactive_mode(self):
        """Интерактивный режим управления"""
        print("\n=== Интерактивный режим ===")
        print("Команды:")
        print("  + или u - Увеличить скорость")
        print("  - или d - Уменьшить скорость")
        print("  p - Изменить направление")
        print("  s - Остановить")
        print("  e - Включить/выключить")
        print("  q - Выход")
        print("  help - Справка")
        print("  test - Автоматический тест")
        
        while True:
            try:
                cmd = input("\nВведите команду: ").strip().lower()
                
                if cmd == 'q' or cmd == 'quit':
                    break
                elif cmd == 'help':
                    self.send_command('?')
                elif cmd == 'test':
                    self.auto_test()
                elif cmd in ['+', 'u']:
                    self.send_command('+')
                elif cmd in ['-', 'd']:
                    self.send_command('-')
                elif cmd == 'p':
                    self.send_command('P')
                elif cmd == 's':
                    self.send_command('S')
                elif cmd == 'e':
                    self.send_command('E')
                elif len(cmd) == 1:
                    self.send_command(cmd.upper())
                else:
                    print("Неизвестная команда. Введите 'help' для справки.")
                    
            except KeyboardInterrupt:
                break
            except Exception as e:
                print(f"Ошибка: {e}")
    
    def auto_test(self):
        """Автоматический тест всех функций"""
        print("\n=== Автоматический тест ===")
        
        tests = [
            ("Включение двигателя", "E"),
            ("Увеличение скорости (1)", "+"),
            ("Увеличение скорости (2)", "+"),
            ("Увеличение скорости (3)", "+"),
            ("Изменение направления", "P"),
            ("Уменьшение скорости (1)", "-"),
            ("Изменение направления обратно", "P"),
            ("Остановка", "S"),
            ("Выключение двигателя", "E")
        ]
        
        for description, command in tests:
            print(f"\n{description}...")
            self.send_command(command)
            time.sleep(2)  # Пауза между командами
        
        print("\nТест завершен!")

def main():
    print("ESP32 Motor Control - Тестовая утилита")
    print("=====================================")
    
    tester = ESP32MotorTester()
    
    try:
        if tester.connect():
            tester.interactive_mode()
    except KeyboardInterrupt:
        print("\nПрерывание пользователем")
    finally:
        tester.disconnect()

if __name__ == "__main__":
    # Проверка зависимостей
    try:
        import bluetooth
    except ImportError:
        print("Ошибка: Требуется установить pybluez")
        print("Выполните: pip install pybluez")
        sys.exit(1)
    
    main()