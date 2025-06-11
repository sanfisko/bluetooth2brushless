#!/bin/bash

# Универсальный скрипт установки с Bluetooth сканированием для проекта esp32-bluetooth-motor-control
# Автор: sanfisko
# Репозиторий: https://github.com/sanfisko/esp32-bluetooth-motor-control
# Версия: install_bt.sh - с поддержкой автоматического поиска BT устройств

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
MAIN_C_FILE="$PROJECT_DIR/main/main.c"

# Функция для вывода заголовка
print_header() {
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║         ESP32 Bluetooth Motor Control Setup (BT)            ║${NC}"
    echo -e "${BLUE}║       github.com/sanfisko/esp32-bluetooth-motor-control     ║${NC}"
    echo -e "${BLUE}╚══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

# Функция для автоматической установки Bluetooth пакетов
install_bluetooth_packages() {
    local os_type=$(uname)
    
    if [ "$os_type" = "Darwin" ]; then
        echo -e "${BLUE}🍎 Установка blueutil для macOS...${NC}"
        if command -v brew >/dev/null 2>&1; then
            brew install blueutil
        else
            echo -e "${RED}❌ Homebrew не найден. Установите brew сначала${NC}"
            return 1
        fi
    else
        echo -e "${BLUE}🐧 Установка Bluetooth пакетов для Linux...${NC}"
        
        # Определяем дистрибутив
        if command -v apt >/dev/null 2>&1; then
            echo -e "${CYAN}Обновление списка пакетов...${NC}"
            sudo apt update
            echo -e "${CYAN}Установка bluetooth, bluez, bluez-tools...${NC}"
            sudo apt install -y bluetooth bluez bluez-tools
        elif command -v yum >/dev/null 2>&1; then
            echo -e "${CYAN}Установка bluez, bluez-tools...${NC}"
            sudo yum install -y bluez bluez-tools
        elif command -v dnf >/dev/null 2>&1; then
            echo -e "${CYAN}Установка bluez, bluez-tools...${NC}"
            sudo dnf install -y bluez bluez-tools
        else
            echo -e "${RED}❌ Неизвестный пакетный менеджер${NC}"
            return 1
        fi
    fi
    
    return 0
}

# Функция для проверки Bluetooth окружения
check_bluetooth_tools() {
    echo -e "${BLUE}🔍 Проверка Bluetooth инструментов...${NC}"
    
    local tools_available=false
    local os_type=$(uname)
    
    # Проверяем blueutil для macOS
    if [ "$os_type" = "Darwin" ] && command -v blueutil >/dev/null 2>&1; then
        echo -e "${GREEN}✅ blueutil найден (macOS)${NC}"
        tools_available=true
    fi
    
    # Проверяем bluetoothctl
    if command -v bluetoothctl >/dev/null 2>&1; then
        echo -e "${GREEN}✅ bluetoothctl найден${NC}"
        tools_available=true
    fi
    
    # Проверяем hcitool
    if command -v hcitool >/dev/null 2>&1; then
        echo -e "${GREEN}✅ hcitool найден${NC}"
        tools_available=true
    fi
    
    # Проверяем rfkill (только для Linux)
    if [ "$os_type" != "Darwin" ] && command -v rfkill >/dev/null 2>&1; then
        echo -e "${GREEN}✅ rfkill найден${NC}"
    fi
    
    if [ "$tools_available" = false ]; then
        echo -e "${YELLOW}⚠️  Bluetooth инструменты не найдены${NC}"
        echo -e "${BLUE}💡 Хотите установить их автоматически?${NC}"
        read -p "Установить Bluetooth пакеты? (Y/n): " -n 1 -r
        echo
        
        if [[ ! $REPLY =~ ^[Nn]$ ]]; then
            if install_bluetooth_packages; then
                echo -e "${GREEN}✅ Bluetooth пакеты установлены${NC}"
                return 0
            else
                echo -e "${RED}❌ Ошибка установки пакетов${NC}"
                echo -e "${CYAN}Установите вручную:${NC}"
                if [ "$os_type" = "Darwin" ]; then
                    echo -e "${YELLOW}macOS: brew install blueutil${NC}"
                else
                    echo -e "${YELLOW}Ubuntu/Debian: sudo apt install bluetooth bluez-tools${NC}"
                    echo -e "${YELLOW}CentOS/RHEL: sudo yum install bluez bluez-tools${NC}"
                fi
                return 1
            fi
        else
            echo -e "${CYAN}Установите вручную:${NC}"
            if [ "$os_type" = "Darwin" ]; then
                echo -e "${YELLOW}macOS: brew install blueutil${NC}"
            else
                echo -e "${YELLOW}Ubuntu/Debian: sudo apt install bluetooth bluez-tools${NC}"
                echo -e "${YELLOW}CentOS/RHEL: sudo yum install bluez bluez-tools${NC}"
            fi
            return 1
        fi
    fi
    
    return 0
}

# Функция для включения Bluetooth
enable_bluetooth() {
    echo -e "${BLUE}📡 Проверка состояния Bluetooth...${NC}"
    
    local os_type=$(uname)
    
    # Для Linux систем
    if [ "$os_type" != "Darwin" ]; then
        # Проверяем и запускаем bluetooth сервис
        if command -v systemctl >/dev/null 2>&1; then
            echo -e "${BLUE}🔧 Проверка bluetooth сервиса...${NC}"
            if ! systemctl is-active --quiet bluetooth; then
                echo -e "${YELLOW}🔌 Запуск bluetooth сервиса...${NC}"
                sudo systemctl start bluetooth
                sleep 2
            fi
            
            if ! systemctl is-enabled --quiet bluetooth; then
                echo -e "${YELLOW}⚙️ Включение автозапуска bluetooth...${NC}"
                sudo systemctl enable bluetooth
            fi
        fi
        
        # Проверяем rfkill
        if command -v rfkill >/dev/null 2>&1; then
            echo -e "${BLUE}🔍 Проверка rfkill блокировок...${NC}"
            if rfkill list bluetooth | grep -q "Soft blocked: yes"; then
                echo -e "${YELLOW}🔓 Снятие программной блокировки Bluetooth...${NC}"
                sudo rfkill unblock bluetooth
                sleep 2
            fi
            if rfkill list bluetooth | grep -q "Hard blocked: yes"; then
                echo -e "${RED}❌ Bluetooth заблокирован аппаратно (проверьте переключатель)${NC}"
                return 1
            fi
        fi
    fi
    
    # Проверяем bluetoothctl
    if command -v bluetoothctl >/dev/null 2>&1; then
        echo -e "${BLUE}🔌 Настройка Bluetooth адаптера...${NC}"
        
        # Включаем адаптер и настраиваем
        (
            echo "power on"
            sleep 2
            echo "agent on"
            echo "default-agent"
            echo "discoverable on"
            echo "pairable on"
            sleep 1
            echo "quit"
        ) | bluetoothctl >/dev/null 2>&1
        
        sleep 2
        
        # Проверяем статус
        local bt_status=$(echo "show" | bluetoothctl 2>/dev/null | grep "Powered:" | awk '{print $2}')
        if [ "$bt_status" = "yes" ]; then
            echo -e "${GREEN}✅ Bluetooth адаптер включен${NC}"
        else
            echo -e "${YELLOW}⚠️ Не удалось включить Bluetooth адаптер${NC}"
            return 1
        fi
    fi
    
    echo -e "${GREEN}✅ Bluetooth готов к сканированию${NC}"
    return 0
}

# Функция для сканирования Bluetooth устройств
scan_bluetooth_devices() {
    echo -e "${BLUE}🔍 Сканирование Bluetooth устройств...${NC}"
    echo -e "${YELLOW}Включите ваш BT13 пульт (долгое нажатие средней кнопки)${NC}"
    echo -e "${CYAN}Сканирование займет 15 секунд...${NC}"
    echo ""
    
    local devices_file="/tmp/bt_devices.txt"
    > "$devices_file"
    
    # Определяем ОС
    local os_type=$(uname)
    
    # macOS - используем blueutil если доступен
    if [ "$os_type" = "Darwin" ] && command -v blueutil >/dev/null 2>&1; then
        echo -e "${BLUE}🍎 Используем blueutil для macOS...${NC}"
        
        # Включаем Bluetooth если выключен
        if [ "$(blueutil -p)" = "0" ]; then
            echo -e "${YELLOW}🔌 Включение Bluetooth...${NC}"
            blueutil -p 1
            sleep 3
        fi
        
        # Сканируем устройства
        blueutil --inquiry 15 2>/dev/null | while read -r line; do
            if [[ "$line" =~ address:\ ([0-9a-fA-F:]+),\ name:\ \"(.*)\" ]]; then
                local mac="${BASH_REMATCH[1]}"
                local name="${BASH_REMATCH[2]}"
                echo "$mac|$name" >> "$devices_file"
            fi
        done
        
    # Используем bluetoothctl если доступен
    elif command -v bluetoothctl >/dev/null 2>&1; then
        echo -e "${BLUE}🐧 Используем bluetoothctl...${NC}"
        
        # Очищаем кэш устройств
        echo -e "${CYAN}Очистка кэша устройств...${NC}"
        echo "remove *" | bluetoothctl >/dev/null 2>&1
        sleep 1
        
        # Запускаем сканирование
        echo -e "${CYAN}Запуск сканирования на 15 секунд...${NC}"
        
        # Создаем временный файл для bluetoothctl
        local bt_script="/tmp/bt_scan.sh"
        cat > "$bt_script" << 'EOF'
#!/bin/bash
{
    echo "scan on"
    sleep 15
    echo "scan off"
    sleep 1
    echo "devices"
    echo "quit"
} | bluetoothctl
EOF
        chmod +x "$bt_script"
        
        # Запускаем сканирование и парсим результат
        "$bt_script" 2>/dev/null | grep "Device" | while read -r line; do
            local mac=$(echo "$line" | awk '{print $2}')
            local name=$(echo "$line" | cut -d' ' -f3-)
            if [ -n "$mac" ] && [ -n "$name" ]; then
                echo "$mac|$name" >> "$devices_file"
            fi
        done
        
        # Удаляем временный файл
        rm -f "$bt_script"
        
        # Дополнительно пробуем получить устройства из кэша
        echo "devices" | bluetoothctl 2>/dev/null | grep "Device" | while read -r line; do
            local mac=$(echo "$line" | awk '{print $2}')
            local name=$(echo "$line" | cut -d' ' -f3-)
            if [ -n "$mac" ] && [ -n "$name" ]; then
                # Проверяем, что устройство еще не добавлено
                if ! grep -q "$mac" "$devices_file" 2>/dev/null; then
                    echo "$mac|$name" >> "$devices_file"
                fi
            fi
        done
        
    # Используем hcitool как резервный вариант
    elif command -v hcitool >/dev/null 2>&1; then
        echo -e "${YELLOW}🔧 Используем hcitool для сканирования...${NC}"
        timeout 15 hcitool scan 2>/dev/null | grep -E "([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}" | while read -r mac name; do
            if [ -n "$mac" ]; then
                echo "$mac|${name:-Unknown Device}" >> "$devices_file"
            fi
        done
    else
        echo -e "${RED}❌ Bluetooth инструменты недоступны${NC}"
        return 1
    fi
    
    # Ждем завершения записи в файл
    sleep 2
    
    # Проверяем результаты
    if [ ! -s "$devices_file" ]; then
        echo -e "${YELLOW}⚠️  Устройства не найдены${NC}"
        echo ""
        echo -e "${BLUE}🔍 Диагностика Bluetooth:${NC}"
        
        # Показываем статус Bluetooth
        if command -v bluetoothctl >/dev/null 2>&1; then
            local bt_status=$(echo "show" | bluetoothctl 2>/dev/null | grep "Powered:" | awk '{print $2}')
            echo -e "${CYAN}• Bluetooth адаптер: ${bt_status:-неизвестно}${NC}"
            
            local scanning=$(echo "show" | bluetoothctl 2>/dev/null | grep "Discovering:" | awk '{print $2}')
            echo -e "${CYAN}• Сканирование: ${scanning:-неизвестно}${NC}"
        fi
        
        # Показываем rfkill статус
        if command -v rfkill >/dev/null 2>&1; then
            echo -e "${CYAN}• rfkill статус:${NC}"
            rfkill list bluetooth | head -3
        fi
        
        echo ""
        echo -e "${CYAN}Убедитесь что:${NC}"
        echo -e "${CYAN}1. BT13 мигает красным+синим (режим поиска)${NC}"
        echo -e "${CYAN}2. BT13 находится рядом (< 5 метров)${NC}"
        echo -e "${CYAN}3. BT13 не подключен к другому устройству${NC}"
        echo -e "${CYAN}4. Bluetooth включен в системе${NC}"
        echo ""
        echo -e "${BLUE}💡 Попробуйте:${NC}"
        echo -e "${CYAN}• Перезапустить BT13 (выключить/включить)${NC}"
        echo -e "${CYAN}• Отключить BT13 от телефона/компьютера${NC}"
        echo -e "${CYAN}• Запустить: sudo systemctl restart bluetooth${NC}"
        echo -e "${CYAN}• Запустить скрипт с sudo${NC}"
        return 1
    fi
    
    return 0
}

# Функция для ручного ввода MAC адреса
manual_mac_input() {
    echo -e "${BLUE}✏️  Ручной ввод MAC адреса${NC}"
    echo -e "${CYAN}Введите MAC адрес вашего BT13 пульта в формате XX:XX:XX:XX:XX:XX${NC}"
    echo -e "${YELLOW}Пример: 8B:EB:75:4E:65:97${NC}"
    echo ""
    
    while true; do
        read -p "MAC адрес: " manual_mac
        
        # Проверяем формат MAC адреса
        if [[ "$manual_mac" =~ ^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$ ]]; then
            echo -e "${GREEN}✅ MAC адрес корректен: $manual_mac${NC}"
            echo "$manual_mac" > /tmp/selected_mac.txt
            return 0
        else
            echo -e "${RED}❌ Неверный формат MAC адреса${NC}"
            echo -e "${CYAN}Используйте формат: XX:XX:XX:XX:XX:XX (например: 8B:EB:75:4E:65:97)${NC}"
            echo ""
        fi
    done
}

# Функция для выбора устройства
select_bluetooth_device() {
    local devices_file="/tmp/bt_devices.txt"
    
    echo -e "${GREEN}✅ Найденные Bluetooth устройства:${NC}"
    echo ""
    
    local devices=()
    local i=1
    
    while IFS='|' read -r mac name; do
        devices+=("$mac|$name")
        echo -e "${CYAN}$i) $name ${YELLOW}($mac)${NC}"
        ((i++))
    done < "$devices_file"
    
    echo ""
    echo -e "${CYAN}$((${#devices[@]}+1))) Ввести MAC адрес вручную${NC}"
    echo -e "${CYAN}0) Пропустить и использовать MAC по умолчанию${NC}"
    echo ""
    
    while true; do
        read -p "Выберите устройство (0-$((${#devices[@]}+1))): " choice
        
        if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 0 ] && [ "$choice" -le "$((${#devices[@]}+1))" ]; then
            if [ "$choice" -eq 0 ]; then
                echo -e "${YELLOW}⚠️  Используется MAC по умолчанию${NC}"
                return 1
            elif [ "$choice" -eq "$((${#devices[@]}+1))" ]; then
                return $(manual_mac_input && echo 0 || echo 1)
            else
                local selected_device="${devices[$((choice-1))]}"
                local selected_mac=$(echo "$selected_device" | cut -d'|' -f1)
                local selected_name=$(echo "$selected_device" | cut -d'|' -f2)
                
                echo -e "${GREEN}✅ Выбрано: $selected_name ($selected_mac)${NC}"
                echo "$selected_mac" > /tmp/selected_mac.txt
                return 0
            fi
        else
            echo -e "${RED}❌ Неверный выбор. Введите число от 0 до $((${#devices[@]}+1))${NC}"
        fi
    done
}

# Функция для обновления MAC адреса в коде
update_mac_in_code() {
    local new_mac="$1"
    
    echo -e "${BLUE}🔧 Обновление MAC адреса в коде...${NC}"
    
    if [ ! -f "$MAIN_C_FILE" ]; then
        echo -e "${RED}❌ Файл main.c не найден: $MAIN_C_FILE${NC}"
        return 1
    fi
    
    # Конвертируем MAC адрес из формата XX:XX:XX:XX:XX:XX в {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}
    local mac_array=$(echo "$new_mac" | sed 's/:/, 0x/g' | sed 's/^/0x/')
    
    # Создаем резервную копию
    cp "$MAIN_C_FILE" "$MAIN_C_FILE.backup"
    
    # Обновляем MAC адрес в коде
    sed -i.tmp "s/static esp_bd_addr_t bt13_addr = {[^}]*}/static esp_bd_addr_t bt13_addr = {$mac_array}/" "$MAIN_C_FILE"
    
    if [ $? -eq 0 ]; then
        rm -f "$MAIN_C_FILE.tmp"
        echo -e "${GREEN}✅ MAC адрес обновлен в коде: {$mac_array}${NC}"
        echo -e "${CYAN}💾 Создана резервная копия: $MAIN_C_FILE.backup${NC}"
        return 0
    else
        echo -e "${RED}❌ Ошибка обновления MAC адреса${NC}"
        mv "$MAIN_C_FILE.backup" "$MAIN_C_FILE"
        return 1
    fi
}

# Функция для Bluetooth сканирования и настройки
bluetooth_setup() {
    echo -e "${BLUE}📡 Настройка Bluetooth устройства${NC}"
    echo ""
    
    # Проверяем инструменты
    if ! check_bluetooth_tools; then
        echo -e "${YELLOW}⚠️  Bluetooth инструменты недоступны${NC}"
        echo ""
        echo -e "${BLUE}💡 Хотите ввести MAC адрес вручную?${NC}"
        read -p "Ввести MAC вручную? (y/N): " -n 1 -r
        echo
        
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            if manual_mac_input; then
                local selected_mac=$(cat /tmp/selected_mac.txt)
                if update_mac_in_code "$selected_mac"; then
                    echo -e "${GREEN}🎉 MAC адрес настроен вручную!${NC}"
                    return 0
                fi
            fi
        fi
        return 1
    fi
    
    # Включаем Bluetooth
    if ! enable_bluetooth; then
        echo -e "${YELLOW}⚠️  Не удалось включить Bluetooth${NC}"
        echo ""
        echo -e "${BLUE}💡 Хотите ввести MAC адрес вручную?${NC}"
        read -p "Ввести MAC вручную? (y/N): " -n 1 -r
        echo
        
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            if manual_mac_input; then
                local selected_mac=$(cat /tmp/selected_mac.txt)
                if update_mac_in_code "$selected_mac"; then
                    echo -e "${GREEN}🎉 MAC адрес настроен вручную!${NC}"
                    return 0
                fi
            fi
        fi
        return 1
    fi
    
    # Сканируем устройства
    if ! scan_bluetooth_devices; then
        echo -e "${YELLOW}⚠️  Сканирование не дало результатов${NC}"
        echo ""
        echo -e "${BLUE}💡 Хотите ввести MAC адрес вручную?${NC}"
        read -p "Ввести MAC вручную? (y/N): " -n 1 -r
        echo
        
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            if manual_mac_input; then
                local selected_mac=$(cat /tmp/selected_mac.txt)
                if update_mac_in_code "$selected_mac"; then
                    echo -e "${GREEN}🎉 MAC адрес настроен вручную!${NC}"
                    return 0
                fi
            fi
        fi
        return 1
    fi
    
    # Выбираем устройство (включая ручной ввод)
    if select_bluetooth_device; then
        local selected_mac=$(cat /tmp/selected_mac.txt)
        if update_mac_in_code "$selected_mac"; then
            echo -e "${GREEN}🎉 Bluetooth устройство настроено успешно!${NC}"
            return 0
        else
            echo -e "${RED}❌ Ошибка настройки устройства${NC}"
            return 1
        fi
    else
        echo -e "${YELLOW}⚠️  Используется конфигурация по умолчанию${NC}"
        return 1
    fi
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
    
    # Очищаем предыдущую сборку
    if [ -d "build" ]; then
        rm -rf build
    fi
    
    # Устанавливаем цель и собираем
    idf.py set-target esp32
    idf.py build
    
    echo -e "${GREEN}✅ Проект скомпилирован успешно!${NC}"
}

# Функция для прошивки ESP32 и запуска монитора
flash_and_monitor() {
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
    
    # Прошиваем и запускаем монитор одной командой
    echo -e "${BLUE}⚡ Прошивка ESP32 на $selected_port со скоростью $FLASH_SPEED и запуск монитора...${NC}"
    echo -e "${YELLOW}Нажмите Ctrl+] для выхода из монитора${NC}"
    echo ""
    
    cd "$PROJECT_DIR"
    
    # Запускаем прошивку и монитор одной командой
    idf.py -p "$selected_port" -b "$FLASH_SPEED" flash monitor | tee /tmp/esp32_monitor.log
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ Прошивка и мониторинг завершены!${NC}"
        return 0
    else
        echo -e "${RED}❌ Ошибка прошивки или мониторинга!${NC}"
        return 1
    fi
}

# Функция для анализа логов мониторинга
analyze_monitor_logs() {
    echo -e "${BLUE}📊 Анализ логов подключения...${NC}"
    
    # Проверяем лог на наличие успешного соединения
    if grep -q "BT13\|bluetooth\|connected\|ready\|Found device\|HID" /tmp/esp32_monitor.log 2>/dev/null; then
        echo -e "${GREEN}✅ Обнаружена активность BT13 в логах!${NC}"
        return 0
    else
        echo -e "${YELLOW}⚠️  Соединение с BT13 не обнаружено в логах${NC}"
        echo -e "${BLUE}Для повторного мониторинга выполните:${NC}"
        echo -e "${YELLOW}idf.py monitor${NC}"
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
    
    # Настройка Bluetooth устройства
    echo ""
    bluetooth_setup
    
    # Активируем ESP-IDF один раз для всех последующих операций
    echo ""
    echo -e "${BLUE}🔧 Активация ESP-IDF окружения...${NC}"
    cd "$PROJECT_DIR"
    source "$ESP_IDF_PATH/export.sh"
    
    # Компилируем проект
    echo ""
    build_project
    
    # Прошиваем ESP32 и запускаем монитор
    echo ""
    if flash_and_monitor; then
        # Анализируем логи
        echo ""
        if analyze_monitor_logs; then
            echo -e "${GREEN}🎉 Установка завершена успешно!${NC}"
            offer_cleanup
        else
            echo -e "${YELLOW}⚠️  Проект прошит, но соединение с BT13 требует проверки${NC}"
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