#!/bin/bash

# 🚀 Автоматическая установка ESP-IDF версии bluetooth2brushless
# Автор: OpenHands
# Дата: 2025-06-09

set -e  # Остановка при любой ошибке

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Функции вывода
print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_header() {
    echo -e "${BLUE}"
    echo "🚗 =============================================="
    echo "   ESP32 BT13 Motor Control - ESP-IDF версия"
    echo "=============================================="
    echo -e "${NC}"
}

# Проверка ESP-IDF окружения
check_esp_idf() {
    print_info "Проверка ESP-IDF окружения..."
    
    if [ -z "$IDF_PATH" ]; then
        print_warning "ESP-IDF окружение не активировано"
        print_info "Попытка активации из стандартного расположения..."
        
        # Попробуем найти ESP-IDF в стандартных местах
        ESP_IDF_PATHS=(
            "$HOME/esp/esp-idf/export.sh"
            "$HOME/.espressif/esp-idf/export.sh"
            "/opt/esp-idf/export.sh"
        )
        
        ESP_IDF_FOUND=false
        for path in "${ESP_IDF_PATHS[@]}"; do
            if [ -f "$path" ]; then
                print_info "Найден ESP-IDF: $path"
                print_info "Активация ESP-IDF окружения..."
                source "$path"
                ESP_IDF_FOUND=true
                break
            fi
        done
        
        if [ "$ESP_IDF_FOUND" = false ]; then
            print_error "ESP-IDF не найден!"
            print_info "Установите ESP-IDF:"
            echo "  mkdir -p ~/esp && cd ~/esp"
            echo "  git clone --recursive https://github.com/espressif/esp-idf.git"
            echo "  cd esp-idf && ./install.sh esp32 && . ./export.sh"
            exit 1
        fi
    fi
    
    # Проверка версии ESP-IDF
    if command -v idf.py &> /dev/null; then
        IDF_VERSION=$(idf.py --version 2>&1 | grep -o "v[0-9]\+\.[0-9]\+\.[0-9]\+")
        print_success "ESP-IDF активирован: $IDF_VERSION"
    else
        print_error "idf.py не найден! Проверьте установку ESP-IDF"
        exit 1
    fi
}

# Установка цели ESP32
set_target() {
    print_info "Установка цели ESP32..."
    if idf.py set-target esp32; then
        print_success "Цель ESP32 установлена"
    else
        print_error "Ошибка установки цели ESP32"
        exit 1
    fi
}

# Сборка проекта
build_project() {
    print_info "Сборка проекта..."
    print_info "Это может занять несколько минут при первой сборке..."
    
    if idf.py build; then
        print_success "Проект собран успешно!"
        return 0
    else
        print_error "Ошибка сборки проекта!"
        print_info "Проверьте логи выше для диагностики проблемы"
        exit 1
    fi
}

# Прошивка ESP32
flash_esp32() {
    local port="$1"
    print_info "Прошивка ESP32 через порт $port..."
    
    # Попытка прошивки
    if idf.py -p "$port" flash; then
        print_success "Прошивка завершена успешно!"
        return 0
    else
        print_error "Ошибка прошивки!"
        print_warning "Возможные причины:"
        echo "  - ESP32 не подключен к порту $port"
        echo "  - Неправильный порт (попробуйте /dev/ttyUSB1, /dev/ttyACM0)"
        echo "  - ESP32 в режиме загрузки (зажмите BOOT при подключении)"
        
        # Предложение повторить прошивку
        echo ""
        read -p "Попробовать прошить еще раз? (y/n): " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            print_info "Повторная попытка прошивки..."
            flash_esp32 "$port"
        else
            print_warning "Прошивка пропущена. Можете прошить вручную:"
            echo "  idf.py -p $port flash"
            return 1
        fi
    fi
}

# Определение порта ESP32
detect_port() {
    print_info "Поиск ESP32..."
    
    # Список возможных портов
    POSSIBLE_PORTS=(
        "/dev/ttyUSB0"
        "/dev/ttyUSB1" 
        "/dev/ttyACM0"
        "/dev/ttyACM1"
        "/dev/cu.usbserial-*"
        "/dev/cu.SLAB_USBtoUART"
    )
    
    for port in "${POSSIBLE_PORTS[@]}"; do
        if ls $port 2>/dev/null; then
            print_success "Найден порт: $port"
            echo "$port"
            return 0
        fi
    done
    
    print_warning "Автоматическое определение порта не удалось"
    print_info "Доступные порты:"
    ls /dev/tty* 2>/dev/null | grep -E "(USB|ACM|usbserial)" || echo "  Порты не найдены"
    
    # Запрос порта у пользователя
    echo ""
    read -p "Введите порт ESP32 (например, /dev/ttyUSB0): " user_port
    if [ -e "$user_port" ]; then
        echo "$user_port"
        return 0
    else
        print_error "Порт $user_port не существует"
        return 1
    fi
}

# Мониторинг
start_monitor() {
    local port="$1"
    print_success "Готово! ESP32 прошит и готов к работе"
    print_info "Подключение BT13:"
    echo "  1. Зарядите BT13"
    echo "  2. Включите BT13 (долгое нажатие средней кнопки)"
    echo "  3. ESP32 найдет BT13 автоматически"
    echo ""
    print_info "Управление:"
    echo "  + короткое  → +1 уровень скорости"
    echo "  - короткое  → -1 уровень скорости"
    echo "  + длинное   → максимум вперед"
    echo "  - длинное   → максимум назад"
    echo "  Средняя     → полная остановка"
    echo ""
    print_info "Запуск мониторинга (Ctrl+] для выхода)..."
    echo ""
    
    # Небольшая задержка перед мониторингом
    sleep 2
    
    # Запуск мониторинга
    idf.py -p "$port" monitor
}

# Основная функция
main() {
    print_header
    
    # Проверка, что мы в правильной директории
    if [ ! -f "main/main.c" ]; then
        print_error "Запустите скрипт из папки esp-idf-version!"
        print_info "cd bluetooth2brushless/esp-idf-version && ./install.sh"
        exit 1
    fi
    
    # Проверка ESP-IDF
    check_esp_idf
    
    # Установка цели (только если нужно)
    if [ ! -f "sdkconfig" ]; then
        set_target
    else
        print_info "Цель ESP32 уже установлена"
    fi
    
    # Сборка проекта
    build_project
    
    # Определение порта
    ESP_PORT=$(detect_port)
    if [ $? -ne 0 ]; then
        print_error "Не удалось определить порт ESP32"
        print_info "Подключите ESP32 и запустите скрипт снова"
        exit 1
    fi
    
    # Прошивка
    if flash_esp32 "$ESP_PORT"; then
        # Мониторинг
        start_monitor "$ESP_PORT"
    else
        print_warning "Прошивка не выполнена"
        print_info "Для мониторинга выполните:"
        echo "  idf.py -p $ESP_PORT monitor"
    fi
}

# Обработка Ctrl+C
trap 'print_warning "Установка прервана пользователем"; exit 1' INT

# Запуск
main "$@"