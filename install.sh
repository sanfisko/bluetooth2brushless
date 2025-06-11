#!/bin/bash

# Универсальный скрипт установки и управления ESP-IDF для проекта bluetooth2brushless
# Автор: sanfisko
# Репозиторий: https://github.com/sanfisko/bluetooth2brushless

set -e

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Константы
ESP_IDF_VERSION="v5.4.1"
ESP_DIR="$HOME/esp"
ESP_IDF_PATH="$ESP_DIR/esp-idf"
PROJECT_DIR="$(pwd)"
FLASH_SPEED="115200"

# Функция для вывода заголовка
print_header() {
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║              ESP32 BT13 Motor Control Setup                  ║${NC}"
    echo -e "${BLUE}║              Репозиторий: github.com/sanfisko                ║${NC}"
    echo -e "${BLUE}╚══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

# Функция для проверки установки ESP-IDF
check_esp_idf() {
    if [ -d "$ESP_IDF_PATH" ]; then
        cd "$ESP_IDF_PATH"
        local current_version=$(git describe --tags --exact-match 2>/dev/null || git rev-parse --short HEAD)
        echo -e "${GREEN}✅ ESP-IDF найден: $current_version${NC}"
        
        # Проверяем, нужно ли обновление
        git fetch --tags >/dev/null 2>&1
        if ! git describe --tags --exact-match >/dev/null 2>&1 || [ "$(git describe --tags --exact-match)" != "$ESP_IDF_VERSION" ]; then
            echo -e "${YELLOW}⚠️  Доступна стабильная версия $ESP_IDF_VERSION${NC}"
            return 1
        else
            echo -e "${GREEN}✅ Установлена актуальная стабильная версия${NC}"
            return 0
        fi
    else
        echo -e "${RED}❌ ESP-IDF не найден${NC}"
        return 1
    fi
}

# Функция для установки ESP-IDF
install_esp_idf() {
    echo -e "${BLUE}📦 Установка ESP-IDF $ESP_IDF_VERSION...${NC}"
    
    # Создаем директорию
    mkdir -p "$ESP_DIR"
    cd "$ESP_DIR"
    
    # Удаляем старую версию если есть
    if [ -d "esp-idf" ]; then
        echo -e "${YELLOW}🗑️  Удаление старой версии...${NC}"
        rm -rf esp-idf
    fi
    
    # Клонируем стабильную версию
    echo -e "${BLUE}📥 Клонирование ESP-IDF $ESP_IDF_VERSION...${NC}"
    git clone --recursive --branch $ESP_IDF_VERSION https://github.com/espressif/esp-idf.git
    
    cd esp-idf
    
    # Устанавливаем инструменты
    echo -e "${BLUE}🔧 Установка инструментов...${NC}"
    ./install.sh esp32
    
    echo -e "${GREEN}✅ ESP-IDF $ESP_IDF_VERSION установлен успешно!${NC}"
}

# Функция для обновления ESP-IDF
update_esp_idf() {
    echo -e "${BLUE}🔄 Обновление ESP-IDF до $ESP_IDF_VERSION...${NC}"
    
    cd "$ESP_IDF_PATH"
    git fetch --tags
    git checkout $ESP_IDF_VERSION
    git submodule update --init --recursive
    
    # Переустанавливаем инструменты
    echo -e "${BLUE}🔧 Обновление инструментов...${NC}"
    ./install.sh esp32
    
    echo -e "${GREEN}✅ ESP-IDF обновлен до $ESP_IDF_VERSION!${NC}"
}

# Функция для компиляции проекта
build_project() {
    echo -e "${BLUE}🔨 Компиляция проекта...${NC}"
    
    cd "$PROJECT_DIR"
    
    # Активируем ESP-IDF
    source "$ESP_IDF_PATH/export.sh"
    
    # Очищаем предыдущую сборку
    if [ -d "build" ]; then
        rm -rf build
    fi
    
    # Устанавливаем цель и собираем
    idf.py set-target esp32
    idf.py build
    
    echo -e "${GREEN}✅ Проект скомпилирован успешно!${NC}"
}

# Функция для прошивки ESP32
flash_esp32() {
    echo -e "${BLUE}⚡ Поиск ESP32...${NC}"
    
    # Ищем доступные порты
    local ports=()
    for port in /dev/ttyUSB* /dev/ttyACM* /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART*; do
        if [ -e "$port" ]; then
            ports+=("$port")
        fi
    done
    
    if [ ${#ports[@]} -eq 0 ]; then
        echo -e "${RED}❌ ESP32 не найден. Подключите устройство и повторите попытку.${NC}"
        return 1
    fi
    
    # Выбираем порт
    local selected_port
    if [ ${#ports[@]} -eq 1 ]; then
        selected_port="${ports[0]}"
        echo -e "${GREEN}✅ Найден ESP32 на порту: $selected_port${NC}"
    else
        echo -e "${YELLOW}Найдено несколько портов:${NC}"
        for i in "${!ports[@]}"; do
            echo -e "${CYAN}$((i+1))) ${ports[i]}${NC}"
        done
        read -p "Выберите порт (1-${#ports[@]}): " choice
        selected_port="${ports[$((choice-1))]}"
    fi
    
    # Прошиваем
    echo -e "${BLUE}⚡ Прошивка ESP32 на $selected_port со скоростью $FLASH_SPEED...${NC}"
    cd "$PROJECT_DIR"
    source "$ESP_IDF_PATH/export.sh"
    
    idf.py -p "$selected_port" -b "$FLASH_SPEED" flash
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ Прошивка завершена успешно!${NC}"
        return 0
    else
        echo -e "${RED}❌ Ошибка прошивки!${NC}"
        return 1
    fi
}

# Функция для мониторинга и проверки BT13
monitor_and_check() {
    echo -e "${BLUE}📺 Запуск монитора для проверки соединения с BT13...${NC}"
    echo -e "${YELLOW}Нажмите Ctrl+] для выхода из монитора${NC}"
    echo ""
    
    cd "$PROJECT_DIR"
    source "$ESP_IDF_PATH/export.sh"
    
    # Запускаем монитор в фоне и проверяем вывод
    timeout 30 idf.py monitor | tee /tmp/esp32_monitor.log &
    local monitor_pid=$!
    
    sleep 10
    
    # Проверяем лог на наличие успешного соединения
    if grep -q "BT13\|bluetooth\|connected\|ready" /tmp/esp32_monitor.log 2>/dev/null; then
        kill $monitor_pid 2>/dev/null || true
        echo -e "${GREEN}✅ Обнаружено соединение с BT13!${NC}"
        return 0
    else
        kill $monitor_pid 2>/dev/null || true
        echo -e "${YELLOW}⚠️  Соединение с BT13 не обнаружено или требует дополнительной настройки${NC}"
        return 1
    fi
}

# Функция для предложения удаления ESP-IDF
offer_cleanup() {
    echo ""
    echo -e "${YELLOW}🧹 Хотите удалить ESP-IDF для экономии места?${NC}"
    echo -e "${CYAN}ESP-IDF занимает около 2GB дискового пространства${NC}"
    echo ""
    read -p "Удалить ESP-IDF? (y/N): " -n 1 -r
    echo
    
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${BLUE}🗑️  Удаление ESP-IDF...${NC}"
        rm -rf "$ESP_DIR"
        echo -e "${GREEN}✅ ESP-IDF удален${NC}"
    else
        echo -e "${GREEN}✅ ESP-IDF сохранен в $ESP_DIR${NC}"
        echo -e "${BLUE}Для повторного использования выполните:${NC}"
        echo -e "${YELLOW}source $ESP_IDF_PATH/export.sh${NC}"
    fi
}

# Основная функция
main() {
    print_header
    
    # Проверяем ESP-IDF
    if check_esp_idf; then
        echo -e "${GREEN}✅ ESP-IDF актуален${NC}"
    else
        echo -e "${YELLOW}🔄 Требуется установка/обновление ESP-IDF${NC}"
        read -p "Продолжить? (Y/n): " -n 1 -r
        echo
        
        if [[ ! $REPLY =~ ^[Nn]$ ]]; then
            if [ -d "$ESP_IDF_PATH" ]; then
                update_esp_idf
            else
                install_esp_idf
            fi
        else
            echo -e "${YELLOW}⚠️  Установка отменена${NC}"
            exit 0
        fi
    fi
    
    # Компилируем проект
    echo ""
    build_project
    
    # Прошиваем ESP32
    echo ""
    if flash_esp32; then
        # Проверяем соединение с BT13
        echo ""
        if monitor_and_check; then
            echo -e "${GREEN}🎉 Установка завершена успешно!${NC}"
            offer_cleanup
        else
            echo -e "${YELLOW}⚠️  Проект прошит, но соединение с BT13 требует проверки${NC}"
            echo -e "${BLUE}Запустите монитор вручную: idf.py monitor${NC}"
        fi
    else
        echo -e "${RED}❌ Ошибка прошивки. Проверьте подключение ESP32${NC}"
        exit 1
    fi
    
    echo ""
    echo -e "${GREEN}✅ Готово!${NC}"
}

# Запуск основной функции
main "$@"