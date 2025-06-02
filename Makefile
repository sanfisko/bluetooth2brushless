# Makefile для проекта bluetooth2brushless
# Требует установленного arduino-cli

# Конфигурация
BOARD = esp32:esp32:esp32
PORT = /dev/ttyUSB0
SKETCH = bluetooth2brushless.ino

# Цели по умолчанию
.PHONY: help install compile upload monitor clean test

help:
	@echo "Доступные команды:"
	@echo "  install  - Установка зависимостей"
	@echo "  compile  - Компиляция скетча"
	@echo "  upload   - Загрузка на ESP32"
	@echo "  monitor  - Мониторинг Serial порта"
	@echo "  clean    - Очистка временных файлов"
	@echo "  test     - Запуск тестов"
	@echo "  all      - Компиляция и загрузка"

install:
	@echo "Установка пакета ESP32..."
	arduino-cli core update-index
	arduino-cli core install esp32:esp32
	@echo "Установка завершена!"

compile:
	@echo "Компиляция $(SKETCH)..."
	arduino-cli compile --fqbn $(BOARD) $(SKETCH)

upload: compile
	@echo "Загрузка на ESP32 (порт: $(PORT))..."
	arduino-cli upload -p $(PORT) --fqbn $(BOARD) $(SKETCH)

monitor:
	@echo "Мониторинг порта $(PORT) (115200 baud)..."
	arduino-cli monitor -p $(PORT) -c baudrate=115200

clean:
	@echo "Очистка временных файлов..."
	rm -rf build/
	rm -f *.hex *.elf *.map *.bin
	@echo "Очистка завершена!"

test:
	@echo "Запуск тестов..."
	@if command -v python3 >/dev/null 2>&1; then \
		python3 test_commands.py; \
	else \
		echo "Python3 не найден. Установите Python для запуска тестов."; \
	fi

all: upload

# Дополнительные утилиты
check-port:
	@echo "Доступные порты:"
	arduino-cli board list

erase-flash:
	@echo "Очистка Flash памяти ESP32..."
	esptool.py --chip esp32 --port $(PORT) erase_flash

info:
	@echo "Информация о проекте:"
	@echo "  Плата: $(BOARD)"
	@echo "  Порт: $(PORT)"
	@echo "  Скетч: $(SKETCH)"
	@echo ""
	@echo "Структура проекта:"
	@ls -la

# Настройка порта (для Linux/macOS)
set-port:
	@read -p "Введите порт (например, /dev/ttyUSB0): " port; \
	sed -i.bak "s|PORT = .*|PORT = $$port|" Makefile
	@echo "Порт обновлен в Makefile"

# Документация
docs:
	@echo "Открытие документации..."
	@if command -v xdg-open >/dev/null 2>&1; then \
		xdg-open README.markdown; \
	elif command -v open >/dev/null 2>&1; then \
		open README.markdown; \
	else \
		echo "Откройте README.markdown в текстовом редакторе"; \
	fi