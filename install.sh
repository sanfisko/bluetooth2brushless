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

# Проверка системных зависимостей
check_system_dependencies() {
    print_info "Проверка системных зависимостей..."
    
    local missing_deps=()
    
    # Проверка git
    if ! command -v git &> /dev/null; then
        missing_deps+=("git")
    fi
    
    # Проверка python3
    if ! command -v python3 &> /dev/null; then
        missing_deps+=("python3")
    fi
    
    # Проверка curl
    if ! command -v curl &> /dev/null; then
        missing_deps+=("curl")
    fi
    
    # Проверка pip
    if ! command -v pip3 &> /dev/null && ! python3 -m pip --version &> /dev/null; then
        missing_deps+=("python3-pip")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Отсутствуют необходимые зависимости: ${missing_deps[*]}"
        print_info "Установите их с помощью пакетного менеджера:"
        echo ""
        echo "Ubuntu/Debian:"
        echo "  sudo apt update && sudo apt install ${missing_deps[*]}"
        echo ""
        echo "CentOS/RHEL/Fedora:"
        echo "  sudo yum install ${missing_deps[*]} (или dnf вместо yum)"
        echo ""
        echo "macOS:"
        echo "  brew install ${missing_deps[*]}"
        echo ""
        exit 1
    else
        print_success "Все системные зависимости установлены"
    fi
}

# Установка ESP-IDF
install_esp_idf() {
    print_info "Установка ESP-IDF..."
    
    # Сохранение текущей директории
    local original_dir=$(pwd)
    
    # Создание директории
    ESP_DIR="$HOME/esp"
    mkdir -p "$ESP_DIR"
    cd "$ESP_DIR"
    
    # Клонирование ESP-IDF
    print_info "Загрузка ESP-IDF (это может занять несколько минут)..."
    if git clone --recursive https://github.com/espressif/esp-idf.git; then
        print_success "ESP-IDF загружен успешно"
    else
        print_error "Ошибка загрузки ESP-IDF"
        cd "$original_dir"
        exit 1
    fi
    
    cd esp-idf
    
    # Установка зависимостей
    print_info "Установка зависимостей ESP-IDF..."
    if ./install.sh esp32; then
        print_success "Зависимости установлены"
    else
        print_error "Ошибка установки зависимостей"
        cd "$original_dir"
        exit 1
    fi
    
    # Активация окружения
    print_info "Активация ESP-IDF окружения..."
    source ./export.sh
    
    # Возврат в исходную директорию
    cd "$original_dir"
    
    print_success "ESP-IDF установлен и активирован!"
}

# Проверка обновлений ESP-IDF
check_esp_idf_updates() {
    local esp_idf_path="$1"
    
    print_info "Проверка обновлений ESP-IDF..."
    
    # Сохранение текущей директории
    local original_dir=$(pwd)
    
    cd "$(dirname "$esp_idf_path")"
    
    # Получение информации о текущей версии
    local current_commit=$(git rev-parse HEAD 2>/dev/null)
    if [ $? -ne 0 ]; then
        print_warning "Не удалось определить текущую версию ESP-IDF"
        cd "$original_dir"
        return 1
    fi
    
    # Попытка получения обновлений с таймаутом
    print_info "Проверка удаленного репозитория..."
    if timeout 30 git fetch origin >/dev/null 2>&1; then
        local latest_commit=$(git rev-parse origin/master 2>/dev/null)
        
        if [ -z "$latest_commit" ]; then
            print_warning "Не удалось получить информацию об обновлениях"
            print_info "Возможно, проблемы с сетью. Продолжаем с текущей версией."
            cd "$original_dir"
            return 0
        fi
        
        if [ "$current_commit" != "$latest_commit" ]; then
            print_warning "Доступны обновления ESP-IDF"
            echo ""
            read -p "Обновить ESP-IDF до последней версии? (y/n): " -n 1 -r
            echo ""
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                print_info "Обновление ESP-IDF..."
                
                # Проверка состояния репозитория
                if git status --porcelain | grep -q .; then
                    print_info "Обнаружены локальные изменения, сохранение..."
                    git stash push -m "Auto-stash before ESP-IDF update" >/dev/null 2>&1
                fi
                
                # Настройка стратегии pull для избежания конфликтов
                git config pull.rebase false >/dev/null 2>&1
                
                # Проверка текущей ветки
                current_branch=$(git branch --show-current 2>/dev/null || echo "master")
                if [ "$current_branch" != "master" ]; then
                    print_info "Переключение на ветку master..."
                    git checkout master >/dev/null 2>&1
                fi
                
                # Сброс к удаленной версии для чистого обновления
                print_info "Сброс к последней версии ESP-IDF..."
                if git reset --hard origin/master >/dev/null 2>&1; then
                    print_success "Сброс выполнен успешно"
                else
                    print_warning "Проблемы со сбросом, пробуем принудительное обновление..."
                    git fetch --all >/dev/null 2>&1
                    git reset --hard origin/master >/dev/null 2>&1
                fi
                
                # Обновление субмодулей
                print_info "Обновление субмодулей ESP-IDF..."
                if timeout 300 git submodule update --init --recursive --force; then
                    print_success "ESP-IDF обновлен успешно"
                    
                    # Переустановка зависимостей
                    print_info "Обновление зависимостей..."
                    if timeout 300 ./install.sh esp32; then
                        print_success "Зависимости обновлены"
                    else
                        print_warning "Ошибка обновления зависимостей, но ESP-IDF обновлен"
                    fi
                    
                    cd "$original_dir"
                    return 0
                else
                    print_error "Ошибка обновления ESP-IDF"
                    echo ""
                    print_warning "Стандартное обновление не удалось"
                    read -p "Попробовать полную переустановку ESP-IDF? (y/n): " -n 1 -r
                    echo ""
                    if [[ $REPLY =~ ^[Yy]$ ]]; then
                        print_info "Полная переустановка ESP-IDF..."
                        cd "$original_dir"
                        
                        # Резервное копирование старой версии
                        if [ -d "$HOME/esp/esp-idf" ]; then
                            print_info "Создание резервной копии..."
                            mv "$HOME/esp/esp-idf" "$HOME/esp/esp-idf-backup-$(date +%Y%m%d-%H%M%S)" 2>/dev/null
                        fi
                        
                        # Переустановка
                        install_esp_idf
                        return $?
                    else
                        cd "$original_dir"
                        return 1
                    fi
                fi
            else
                print_info "Обновление пропущено"
            fi
        else
            print_success "ESP-IDF уже актуальной версии"
        fi
    else
        print_warning "Не удалось проверить обновления (таймаут сети)"
        print_info "Продолжаем с текущей версией ESP-IDF"
    fi
    
    # Возврат в исходную директорию
    cd "$original_dir"
}

# Проверка ESP-IDF окружения
check_esp_idf() {
    print_info "Проверка ESP-IDF окружения..."
    
    # Попробуем найти ESP-IDF в стандартных местах
    ESP_IDF_PATHS=(
        "$HOME/esp/esp-idf/export.sh"
        "$HOME/.espressif/esp-idf/export.sh"
        "/opt/esp-idf/export.sh"
    )
    
    ESP_IDF_FOUND=false
    ESP_IDF_PATH=""
    
    for path in "${ESP_IDF_PATHS[@]}"; do
        if [ -f "$path" ]; then
            ESP_IDF_PATH="$path"
            ESP_IDF_FOUND=true
            break
        fi
    done
    
    if [ "$ESP_IDF_FOUND" = false ]; then
        print_warning "ESP-IDF не найден в системе"
        echo ""
        read -p "Установить ESP-IDF автоматически? (y/n): " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            install_esp_idf
        else
            print_error "ESP-IDF необходим для работы проекта"
            print_info "Установите ESP-IDF вручную:"
            echo "  mkdir -p ~/esp && cd ~/esp"
            echo "  git clone --recursive https://github.com/espressif/esp-idf.git"
            echo "  cd esp-idf && ./install.sh esp32 && . ./export.sh"
            exit 1
        fi
    else
        print_success "Найден ESP-IDF: $ESP_IDF_PATH"
        
        # Проверка обновлений (с возможностью пропуска)
        if [ "$SKIP_UPDATES" = true ]; then
            print_info "Проверка обновлений пропущена (режим быстрой установки)"
        else
            echo ""
            read -p "Проверить обновления ESP-IDF? (y/n/s для пропуска): " -n 1 -r
            echo ""
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                check_esp_idf_updates "$ESP_IDF_PATH"
            elif [[ $REPLY =~ ^[Ss]$ ]]; then
                print_info "Проверка обновлений пропущена"
            else
                print_info "Проверка обновлений пропущена"
            fi
        fi
        
        # Активация окружения
        if [ -z "$IDF_PATH" ]; then
            print_info "Активация ESP-IDF окружения..."
            source "$ESP_IDF_PATH"
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
    local baud_rate="${2:-460800}"  # По умолчанию 460800, можно передать другую скорость
    
    print_info "Прошивка ESP32 через порт $port (скорость: $baud_rate)..."
    
    # Попытка прошивки с указанной скоростью
    if idf.py -p "$port" -b "$baud_rate" flash; then
        print_success "Прошивка завершена успешно!"
        return 0
    else
        print_warning "Стандартная прошивка не удалась, пробуем альтернативный способ..."
        
        # Альтернативный способ прошивки через esptool напрямую
        if [ -f "build/bootloader/bootloader.bin" ] && [ -f "build/bt13_motor_control.bin" ] && [ -f "build/partition_table/partition-table.bin" ]; then
            print_info "Прошивка через esptool напрямую..."
            if python -m esptool --chip esp32 -p "$port" -b "$baud_rate" --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size 2MB 0x1000 build/bootloader/bootloader.bin 0x10000 build/bt13_motor_control.bin 0x8000 build/partition_table/partition-table.bin; then
                print_success "Альтернативная прошивка завершена успешно!"
                return 0
            fi
        fi
        print_error "Ошибка прошивки!"
        print_warning "Возможные причины:"
        echo "  - ESP32 не подключен к порту $port"
        echo "  - Неправильный порт (попробуйте /dev/ttyUSB1, /dev/ttyACM0)"
        echo "  - ESP32 в режиме загрузки (зажмите BOOT при подключении)"
        echo "  - Проблемы с кабелем USB"
        

        
        # Предложение повторить прошивку
        echo ""
        read -p "Попробовать прошить еще раз? (y/n): " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            print_info "Повторная попытка прошивки..."
            flash_esp32 "$port" "$baud_rate"
        else
            print_warning "Прошивка пропущена. Можете прошить вручную:"
            echo "  idf.py -p $port -b $baud_rate flash"
            return 1
        fi
    fi
}

# Определение порта ESP32
detect_port() {
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
        if ls $port 2>/dev/null >/dev/null; then
            echo "$port"
            return 0
        fi
    done
    
    # Если автоматическое определение не удалось
    return 1
}

# Анализ логов для определения успешного подключения
analyze_monitor_logs() {
    local log_file="$1"
    
    print_info "Анализ логов подключения..."
    
    # Проверяем наличие признаков успешного подключения к BT13
    local connection_patterns=(
        "BT13.*connected"
        "HID.*connected" 
        "Connection established"
        "Device connected.*BT13"
        "GAP_BLE_CONN_EST_EVT"
        "ESP_HIDD_CONNECT_EVT"
    )
    
    for pattern in "${connection_patterns[@]}"; do
        if grep -q "$pattern" "$log_file" 2>/dev/null; then
            print_success "Найден паттерн подключения: $pattern"
            return 0
        fi
    done
    
    # Проверяем наличие HID команд (признак работающего подключения)
    local hid_patterns=(
        "HID Usage: 0x00B[0-9A-F]"
        "Command:.*Short"
        "Command:.*Long" 
        "Speed level.*[0-9]"
        "PWM:.*[0-9]"
        "Direction:.*FORWARD\|REVERSE\|STOP"
        "State:.*ON\|OFF"
    )
    
    local hid_found=0
    for pattern in "${hid_patterns[@]}"; do
        if grep -q "$pattern" "$log_file" 2>/dev/null; then
            print_success "Найдена HID активность: $pattern"
            hid_found=1
        fi
    done
    
    if [ $hid_found -eq 1 ]; then
        return 0
    fi
    
    # Проверяем наличие MAC адреса BT13 в логах
    if grep -q "8B:EB:75:4E:65:97\|8b:eb:75:4e:65:97" "$log_file" 2>/dev/null; then
        print_success "Найден MAC адрес BT13 в логах"
        return 0
    fi
    
    print_warning "Признаки подключения к BT13 не найдены"
    return 1  # Подключение не обнаружено
}

# Функция удаления ESP-IDF
cleanup_esp_idf() {
    print_info "Анализ использования диска..."
    
    # Проверяем размер ESP-IDF
    if [ -d "$HOME/esp/esp-idf" ]; then
        local esp_size=$(du -sh "$HOME/esp/esp-idf" 2>/dev/null | cut -f1)
        print_info "ESP-IDF занимает: $esp_size"
        echo ""
        print_warning "ESP32 успешно прошит и работает с BT13!"
        print_info "ESP-IDF больше не нужен для работы устройства."
        print_info "Вы можете удалить его для освобождения места (~2-3 ГБ)."
        echo ""
        print_warning "⚠️  ВНИМАНИЕ: После удаления ESP-IDF вы не сможете:"
        echo "  - Перепрошивать ESP32 без повторной установки ESP-IDF"
        echo "  - Изменять код проекта"
        echo "  - Обновлять прошивку"
        echo ""
        print_info "Варианты действий:"
        echo "  1) Удалить ESP-IDF (освободить $esp_size)"
        echo "  2) Переместить в архив (~/.esp-idf-backup)"
        echo "  3) Оставить как есть"
        echo ""
        read -p "Ваш выбор (1-3): " -n 1 -r
        echo ""
        
        case $REPLY in
            1)
                print_info "Удаление ESP-IDF..."
                if rm -rf "$HOME/esp/esp-idf"; then
                    print_success "ESP-IDF удален! Освобождено: $esp_size"
                    print_info "Для повторной прошивки запустите ./install.sh - ESP-IDF установится автоматически"
                else
                    print_error "Ошибка удаления ESP-IDF"
                fi
                ;;
            2)
                print_info "Перемещение ESP-IDF в архив..."
                mkdir -p "$HOME/.esp-idf-backup"
                if mv "$HOME/esp/esp-idf" "$HOME/.esp-idf-backup/esp-idf-$(date +%Y%m%d)"; then
                    print_success "ESP-IDF перемещен в архив: ~/.esp-idf-backup/"
                    print_info "Для восстановления: mv ~/.esp-idf-backup/esp-idf-* ~/esp/esp-idf"
                else
                    print_error "Ошибка перемещения ESP-IDF"
                fi
                ;;
            *)
                print_info "ESP-IDF оставлен без изменений"
                ;;
        esac
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
    print_info "Мониторинг будет анализировать подключение к BT13..."
    echo ""
    
    # Небольшая задержка перед мониторингом
    sleep 2
    
    # Создание временного файла для логов
    local log_file="/tmp/esp32_monitor_$$.log"
    
    # Запуск мониторинга с сохранением логов
    print_info "Для выхода из мониторинга нажмите Ctrl+]"
    echo ""
    idf.py -p "$port" monitor | tee "$log_file"
    
    echo ""
    print_info "Мониторинг завершен. Анализ результатов..."
    
    # Анализ логов
    if analyze_monitor_logs "$log_file"; then
        print_success "Обнаружено успешное подключение к BT13!"
        cleanup_esp_idf
    else
        print_warning "Подключение к BT13 не обнаружено в логах"
        print_info "Возможные причины:"
        echo "  - BT13 не включен или разряжен"
        echo "  - Неправильный MAC адрес в коде"
        echo "  - Слишком короткое время мониторинга"
        echo ""
        
        # Предложение принудительного удаления
        echo ""
        read -p "Удалить ESP-IDF принудительно? (y/n): " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            print_info "Принудительное удаление ESP-IDF..."
            cleanup_esp_idf
        else
            print_info "ESP-IDF оставлен для повторных попыток"
        fi
    fi
    
    # Очистка временного файла
    rm -f "$log_file"
}

# Скорость прошивки (фиксированная)
get_baud_rate() {
    echo "115200"
}

# Основная функция
main() {
    print_header
    
    # Проверка параметров командной строки
    SKIP_UPDATES=false
    if [ "$1" = "--skip-updates" ] || [ "$1" = "-s" ]; then
        SKIP_UPDATES=true
        print_info "Режим быстрой установки (пропуск проверки обновлений)"
    fi
    
    # Проверка, что мы в правильной директории
    if [ ! -f "main/main.c" ]; then
        print_error "Запустите скрипт из корневой папки проекта!"
        print_info "cd bluetooth2brushless && ./install.sh"
        exit 1
    fi
    
    # Проверка системных зависимостей
    check_system_dependencies
    
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
    print_info "Поиск ESP32..."
    ESP_PORT=$(detect_port)
    if [ $? -ne 0 ]; then
        print_warning "Автоматическое определение порта не удалось"
        print_info "Доступные порты:"
        ls /dev/tty* 2>/dev/null | grep -E "(USB|ACM|usbserial)" || echo "  Порты не найдены"
        
        # Запрос порта у пользователя
        echo ""
        read -p "Введите порт ESP32 (например, /dev/ttyUSB0): " ESP_PORT
        if [ ! -e "$ESP_PORT" ]; then
            print_error "Порт $ESP_PORT не существует"
            exit 1
        fi
    else
        print_success "Найден порт: $ESP_PORT"
    fi
    
    # Установка скорости прошивки
    BAUD_RATE=$(get_baud_rate)
    print_info "Скорость прошивки: $BAUD_RATE (надежная для всех кабелей)"
    
    # Прошивка
    if flash_esp32 "$ESP_PORT" "$BAUD_RATE"; then
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