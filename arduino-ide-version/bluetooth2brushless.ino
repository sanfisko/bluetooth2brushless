/*
 * ESP32 HID Host для подключения к пульту BT13
 * Управление бесщеточным двигателем через HID команды
 *
 * ВАЖНО: Этот код требует ESP32 Arduino Core версии 3.0.0 или выше
 * Основан на рабочей версии ESP-IDF с полной поддержкой HID Host API
 * Обновлен для Arduino Core 3.x с правильными библиотеками и проверками состояния Bluetooth
 *
 * Автор: OpenHands
 * Дата: 2025-06-10
 */

#include <Arduino.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_hidh.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Пины для управления двигателем
#define MOTOR_SPEED_PIN     25
#define MOTOR_DIR_PIN       26
#define LED_PIN             2

// PWM настройки для Arduino Core 3.x
#define PWM_FREQUENCY       1000
#define PWM_RESOLUTION      8

// MAC адрес пульта BT13
static esp_bd_addr_t bt13_addr = {0x8B, 0xEB, 0x75, 0x4E, 0x65, 0x97};

// Переменные управления двигателем
static int speed_level = 0;        // Уровень скорости от -10 до +10 (0 = стоп)
static bool motor_enabled = false; // Состояние двигателя
static bool long_press_active = false; // Флаг длинного нажатия

// Настройки управления
static const int max_speed_level = 10;   // Максимальный уровень скорости
static const int pwm_per_level = 25;     // PWM на уровень (255/10 ≈ 25)

// Переменные для отслеживания длинных нажатий
static uint32_t long_press_start_time = 0;
static uint32_t last_long_press_event_time = 0;
static uint16_t long_press_button = 0;
static bool is_long_press_detected = false;

// Таймаут для определения реального отпускания длительного нажатия (мс)
#define LONG_PRESS_RELEASE_TIMEOUT_MS 200

// HID Usage коды для кнопок BT13
#define HID_USAGE_SHORT_PLUS    0x0004  // Короткое нажатие +
#define HID_USAGE_SHORT_MINUS   0x0008  // Короткое нажатие -
#define HID_USAGE_STOP          0x0010  // Кнопка STOP
#define HID_USAGE_LONG_PLUS     0x0001  // Длительное нажатие + (повторяющиеся события)
#define HID_USAGE_LONG_MINUS    0x0002  // Длительное нажатие - (повторяющиеся события)

// HID Host переменные
static bool bt13_connected = false;
static bool restart_scan_needed = false;
static bool scanning_in_progress = false;

// Переменные для автоматической остановки мотора
static uint32_t disconnection_start_time = 0;
static const uint32_t MOTOR_STOP_TIMEOUT_MS = 10000; // 10 секунд

// Bluetooth состояние
static bool bluetooth_initialized = false;
static bool bluetooth_enabled = false;
static bool gap_callback_registered = false;
static bool hidh_initialized = false;

// Переменные для ограничения попыток перезапуска
static uint32_t last_restart_attempt = 0;
static int restart_attempts = 0;
static const int MAX_RESTART_ATTEMPTS = 3;
static const uint32_t RESTART_COOLDOWN_MS = 30000; // 30 секунд между попытками

// Функции управления двигателем
static void motor_init(void);
static void motor_update_state(void);
static void print_motor_status(void);

// Функции обработки кнопок
static void short_press_plus(void);
static void short_press_minus(void);
static void start_long_press_plus(void);
static void start_long_press_minus(void);
static void end_long_press(void);
static void motor_stop_command(void);
static void motor_stop(void);
static void led_blink(int times, int delay_ms);

// Bluetooth функции
static bool check_bluetooth_state(void);
static bool initialize_bluetooth(void);
static void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
static void hid_host_cb(void *handler_args, const char *event_name, int32_t event_id, void *param);
static void start_scan_for_bt13(void);
static void connection_monitor_task(void *pvParameters);

void setup() {
    Serial.begin(115200);
    Serial.println("=== ESP32 HID Host Motor Control v3.x ===");
    Serial.println("System initialization...");

    // Инициализация двигателя
    motor_init();
    Serial.println("Motor initialized");

    // Инициализация таймера отключения (система стартует без соединения)
    disconnection_start_time = millis();

    // Проверка и инициализация Bluetooth
    if (!check_bluetooth_state()) {
        Serial.println("ERROR: Bluetooth state check failed!");
        return;
    }

    if (!initialize_bluetooth()) {
        Serial.println("ERROR: Bluetooth initialization failed!");
        return;
    }

    Serial.println("Bluetooth initialized successfully");
    Serial.printf("Searching for BT13 remote (MAC: %02X:%02X:%02X:%02X:%02X:%02X)...\n",
             bt13_addr[0], bt13_addr[1], bt13_addr[2],
             bt13_addr[3], bt13_addr[4], bt13_addr[5]);

    // Начать поиск BT13
    start_scan_for_bt13();

    // Создать задачу мониторинга соединения
    xTaskCreate(connection_monitor_task, "connection_monitor", 2048, NULL, 5, NULL);

    Serial.println("System ready!");
}

void loop() {
    // Проверка состояния Bluetooth с ограничением попыток
    if (!bluetooth_enabled) {
        uint32_t current_time = millis();
        
        // Проверяем, не превышено ли количество попыток
        if (restart_attempts >= MAX_RESTART_ATTEMPTS) {
            // Проверяем, прошло ли достаточно времени для сброса счетчика
            if (current_time - last_restart_attempt > RESTART_COOLDOWN_MS) {
                restart_attempts = 0;
                Serial.println("Restart attempts counter reset after cooldown");
            } else {
                // Слишком много попыток, ждем
                delay(10000); // Ждем 10 секунд
                return;
            }
        }
        
        Serial.printf("WARNING: Bluetooth not enabled, attempting restart... (attempt %d/%d)\n", 
                     restart_attempts + 1, MAX_RESTART_ATTEMPTS);
        
        last_restart_attempt = current_time;
        restart_attempts++;
        
        if (initialize_bluetooth()) {
            Serial.println("Bluetooth restarted successfully");
            restart_attempts = 0; // Сброс счетчика при успехе
        } else {
            Serial.printf("Bluetooth restart failed (attempt %d/%d)\n", restart_attempts, MAX_RESTART_ATTEMPTS);
            delay(5000); // Ждем 5 секунд перед повторной попыткой
            return;
        }
    }

    // Проверка таймаута длительного нажатия
    if (is_long_press_detected && last_long_press_event_time > 0) {
        uint32_t current_time = millis();
        uint32_t time_since_last_event = current_time - last_long_press_event_time;
        
        if (time_since_last_event > LONG_PRESS_RELEASE_TIMEOUT_MS) {
            // Таймаут - считаем что кнопка отпущена
            Serial.printf("Long press timeout detected (%lu ms)\n", time_since_last_event);
            end_long_press();
            
            // Сброс состояния длительного нажатия
            long_press_button = 0;
            is_long_press_detected = false;
            long_press_start_time = 0;
            last_long_press_event_time = 0;
        }
    }
    
    // Индикация состояния через LED
    if (bt13_connected && motor_enabled) {
        led_blink(1, 100);
    } else if (bt13_connected) {
        led_blink(1, 500);
    }

    delay(50); // Уменьшили задержку для более точной обработки
}

static bool check_bluetooth_state(void)
{
    Serial.println("Checking Bluetooth state...");
    
    // Проверка состояния контроллера Bluetooth
    esp_bt_controller_status_t controller_status = esp_bt_controller_get_status();
    Serial.printf("BT Controller status: %d\n", controller_status);
    
    switch (controller_status) {
        case ESP_BT_CONTROLLER_STATUS_IDLE:
            Serial.println("BT Controller: IDLE (not initialized)");
            break;
        case ESP_BT_CONTROLLER_STATUS_INITED:
            Serial.println("BT Controller: INITIALIZED");
            break;
        case ESP_BT_CONTROLLER_STATUS_ENABLED:
            Serial.println("BT Controller: ENABLED");
            break;
        default:
            Serial.println("BT Controller: INVALID STATUS");
            return false;
    }
    
    // Проверка состояния Bluedroid
    esp_bluedroid_status_t bluedroid_status = esp_bluedroid_get_status();
    Serial.printf("Bluedroid status: %d\n", bluedroid_status);
    
    switch (bluedroid_status) {
        case ESP_BLUEDROID_STATUS_UNINITIALIZED:
            Serial.println("Bluedroid: UNINITIALIZED");
            break;
        case ESP_BLUEDROID_STATUS_INITIALIZED:
            Serial.println("Bluedroid: INITIALIZED");
            break;
        case ESP_BLUEDROID_STATUS_ENABLED:
            Serial.println("Bluedroid: ENABLED");
            break;
        default:
            Serial.println("Bluedroid: UNKNOWN STATUS");
            break;
    }
    
    Serial.println("Bluetooth state check completed");
    return true;
}

static bool initialize_bluetooth(void)
{
    esp_err_t ret;
    
    Serial.println("Starting Bluetooth initialization...");
    
    // Проверяем текущее состояние контроллера
    esp_bt_controller_status_t controller_status = esp_bt_controller_get_status();
    esp_bluedroid_status_t bluedroid_status = esp_bluedroid_get_status();
    
    // Если Bluetooth уже полностью инициализирован, просто возвращаем успех
    if (controller_status == ESP_BT_CONTROLLER_STATUS_ENABLED && 
        bluedroid_status == ESP_BLUEDROID_STATUS_ENABLED) {
        Serial.println("Bluetooth already fully initialized");
        bluetooth_initialized = true;
        bluetooth_enabled = true;
        return true;
    }
    
    // Инициализация контроллера только если он не инициализирован
    if (controller_status == ESP_BT_CONTROLLER_STATUS_IDLE) {
        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        
        Serial.println("Initializing BT controller...");
        ret = esp_bt_controller_init(&bt_cfg);
        if (ret != ESP_OK) {
            Serial.printf("BT controller init error: %s\n", esp_err_to_name(ret));
            return false;
        }
        Serial.println("BT controller initialized");
    } else if (controller_status == ESP_BT_CONTROLLER_STATUS_ENABLED) {
        Serial.println("BT controller already enabled");
    }
    
    // Включение контроллера только если он инициализирован, но не включен
    controller_status = esp_bt_controller_get_status();
    if (controller_status == ESP_BT_CONTROLLER_STATUS_INITED) {
        Serial.println("Enabling BT controller...");
        ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
        if (ret != ESP_OK) {
            Serial.printf("BT controller enable error: %s\n", esp_err_to_name(ret));
            return false;
        }
        Serial.println("BT controller enabled");
    }
    
    // Инициализация Bluedroid только если он не инициализирован
    bluedroid_status = esp_bluedroid_get_status();
    if (bluedroid_status == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
        Serial.println("Initializing Bluedroid...");
        ret = esp_bluedroid_init();
        if (ret != ESP_OK) {
            Serial.printf("Bluedroid init error: %s\n", esp_err_to_name(ret));
            return false;
        }
        Serial.println("Bluedroid initialized");
    } else if (bluedroid_status == ESP_BLUEDROID_STATUS_ENABLED) {
        Serial.println("Bluedroid already enabled");
    }
    
    // Включение Bluedroid только если он инициализирован, но не включен
    bluedroid_status = esp_bluedroid_get_status();
    if (bluedroid_status == ESP_BLUEDROID_STATUS_INITIALIZED) {
        Serial.println("Enabling Bluedroid...");
        ret = esp_bluedroid_enable();
        if (ret != ESP_OK) {
            Serial.printf("Bluedroid enable error: %s\n", esp_err_to_name(ret));
            return false;
        }
        Serial.println("Bluedroid enabled");
    }
    
    // Регистрация GAP callback только если еще не зарегистрирован
    if (!gap_callback_registered) {
        Serial.println("Registering GAP callback...");
        ret = esp_bt_gap_register_callback(bt_gap_cb);
        if (ret != ESP_OK) {
            Serial.printf("GAP callback registration error: %s\n", esp_err_to_name(ret));
            return false;
        }
        gap_callback_registered = true;
        Serial.println("GAP callback registered");
    } else {
        Serial.println("GAP callback already registered");
    }
    
    // Инициализация HID Host только если еще не инициализирован
    if (!hidh_initialized) {
        Serial.println("Initializing HID Host...");
        esp_hidh_config_t hidh_config = {
            .callback = hid_host_cb,
            .event_stack_size = 4096,
            .callback_arg = NULL,
        };
        ret = esp_hidh_init(&hidh_config);
        if (ret != ESP_OK) {
            Serial.printf("HID Host init error: %s\n", esp_err_to_name(ret));
            return false;
        }
        hidh_initialized = true;
        Serial.println("HID Host initialized");
    } else {
        Serial.println("HID Host already initialized");
    }
    
    bluetooth_initialized = true;
    bluetooth_enabled = true;
    
    Serial.println("Bluetooth initialization completed successfully");
    return true;
}

static void motor_init(void)
{
    // Настройка PWM для скорости (Arduino Core 3.x API)
    Serial.println("Initializing motor PWM...");
    if (!ledcAttach(MOTOR_SPEED_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
        Serial.println("ERROR: Failed to initialize PWM!");
        return;
    }
    ledcWrite(MOTOR_SPEED_PIN, 0);
    Serial.println("PWM initialized successfully");

    // Настройка пина направления
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    digitalWrite(MOTOR_DIR_PIN, HIGH);

    // Настройка LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Начальное состояние
    motor_update_state();
    Serial.println("Motor initialization completed");
}

static void motor_update_state(void)
{
    // Вычисление PWM и направления на основе уровня скорости
    int actual_speed = 0;
    bool forward = true;

    if (motor_enabled && speed_level != 0) {
        actual_speed = abs(speed_level) * pwm_per_level;
        forward = (speed_level > 0);
    }

    // Обновление PWM для скорости
    ledcWrite(MOTOR_SPEED_PIN, actual_speed);

    // Обновление направления
    digitalWrite(MOTOR_DIR_PIN, forward ? HIGH : LOW);

    Serial.printf("State: %s | Level: %d/%d | PWM: %d/255 | Direction: %s%s\n",
             motor_enabled ? "ON" : "OFF",
             speed_level, max_speed_level,
             actual_speed,
             forward ? "FORWARD" : "BACKWARD",
             long_press_active ? " | LONG PRESS" : "");
}

static void print_motor_status(void)
{
    if (!motor_enabled || speed_level == 0) {
        Serial.println("Stopped");
    } else {
        int percentage = (abs(speed_level) * 100) / max_speed_level;
        const char* direction = (speed_level > 0) ? "forward" : "backward";
        Serial.printf("Running %s at %d%%\n", direction, percentage);
    }
}

// Короткое нажатие + : добавляет 10% к скорости
static void short_press_plus(void)
{
    if (speed_level < max_speed_level) {
        speed_level++;
        motor_enabled = (speed_level != 0);
        motor_update_state();
        int percentage = (speed_level * 100) / max_speed_level;
        Serial.printf("Short +: Speed level = %d (%d%% forward)\n", speed_level, percentage);
        print_motor_status();
    } else {
        Serial.println("Short +: Already at maximum forward speed");
    }
}

// Короткое нажатие - : убавляет 10% от скорости, может переключить направление
static void short_press_minus(void)
{
    if (speed_level > -max_speed_level) {
        speed_level--;
        motor_enabled = (speed_level != 0);
        motor_update_state();
        
        if (speed_level == 0) {
            Serial.println("Short -: Motor stopped");
        } else {
            int percentage = (abs(speed_level) * 100) / max_speed_level;
            const char* direction = (speed_level > 0) ? "forward" : "backward";
            Serial.printf("Short -: Speed level = %d (%d%% %s)\n", speed_level, percentage, direction);
        }
        print_motor_status();
    } else {
        Serial.println("Short -: Already at maximum backward speed");
    }
}

// Длительное нажатие + : мгновенно 100% вперед
static void start_long_press_plus(void)
{
    if (!long_press_active) {
        long_press_active = true;
        speed_level = max_speed_level;
        motor_enabled = true;
        motor_update_state();
        Serial.println("Long + started: 100% forward speed");
        print_motor_status();
    }
}

// Длительное нажатие - : мгновенно 100% назад
static void start_long_press_minus(void)
{
    if (!long_press_active) {
        long_press_active = true;
        speed_level = -max_speed_level;
        motor_enabled = true;
        motor_update_state();
        Serial.println("Long - started: 100% backward speed");
        print_motor_status();
    }
}

// Отпускание длительного нажатия: полная остановка
static void end_long_press(void)
{
    if (long_press_active) {
        long_press_active = false;
        speed_level = 0;
        motor_enabled = false;
        motor_update_state();
        Serial.println("Long press released: Full stop");
        print_motor_status();
    }
}

// Кнопка STOP: полная остановка
static void motor_stop_command(void)
{
    long_press_active = false;
    speed_level = 0;
    motor_enabled = false;
    motor_update_state();
    Serial.println("STOP button: Motor stopped");
    print_motor_status();
}

static void motor_stop(void)
{
    speed_level = 0;
    motor_enabled = false;
    long_press_active = false;
    motor_update_state();
    Serial.println("Motor stopped");
}

static void led_blink(int times, int delay_ms)
{
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delay_ms);
        digitalWrite(LED_PIN, LOW);
        delay(delay_ms);
    }
}

static void start_scan_for_bt13(void)
{
    if (!bluetooth_enabled) {
        Serial.println("Cannot start scan: Bluetooth not enabled");
        return;
    }

    if (scanning_in_progress) {
        Serial.println("Scan already in progress, skipping...");
        return;
    }

    if (bt13_connected) {
        Serial.println("BT13 already connected, scan not needed");
        return;
    }

    Serial.printf("Searching for BT13 remote (MAC: %02X:%02X:%02X:%02X:%02X:%02X)...\n",
             bt13_addr[0], bt13_addr[1], bt13_addr[2],
             bt13_addr[3], bt13_addr[4], bt13_addr[5]);

    scanning_in_progress = true;
    esp_err_t ret = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    if (ret != ESP_OK) {
        Serial.printf("Error starting discovery: %s\n", esp_err_to_name(ret));
        scanning_in_progress = false;
    }
}

static void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT: {
        Serial.printf("Found device: %02x:%02x:%02x:%02x:%02x:%02x\n",
                 param->disc_res.bda[0], param->disc_res.bda[1], param->disc_res.bda[2],
                 param->disc_res.bda[3], param->disc_res.bda[4], param->disc_res.bda[5]);

        // Проверяем, это ли наш BT13
        if (memcmp(param->disc_res.bda, bt13_addr, ESP_BD_ADDR_LEN) == 0) {
            Serial.println("Found BT13! Stopping discovery...");
            esp_bt_gap_cancel_discovery();

            // Подключаемся к BT13
            Serial.println("Connecting to BT13...");
            esp_hidh_dev_t *dev = esp_hidh_dev_open(param->disc_res.bda, ESP_HID_TRANSPORT_BT, 0);
            if (dev == NULL) {
                Serial.println("Failed to connect to BT13");
            }
        }
        break;
    }
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            Serial.println("Device discovery completed");
            scanning_in_progress = false;
        } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
            Serial.println("Device discovery started");
            scanning_in_progress = true;
        }
        break;
    default:
        break;
    }
}

static void hid_host_cb(void *handler_args, const char *event_name, int32_t event_id, void *param)
{
    Serial.printf("HID Host event: %s (ID: %ld)\n", event_name ? event_name : "unknown", event_id);

    // В ESP-IDF v5.1+ (Arduino Core 3.x) используем event_id для определения типа события
    switch (event_id) {
    case 0: // OPEN_EVENT
        Serial.println("BT13 connected successfully!");
        bt13_connected = true;
        disconnection_start_time = 0; // Reset disconnection timer
        Serial.println("Ready to receive commands from remote");
        led_blink(3, 200);
        break;

    case 1: // CLOSE_EVENT
    case 4: // CLOSE_EVENT/DISCONNECT_EVENT (альтернативный ID)
        bt13_connected = false;
        disconnection_start_time = millis(); // Запомнить время отключения
        Serial.println("BT13 disconnected. Restart scan scheduled...");
        motor_stop(); // Остановить двигатель при отключении
        led_blink(5, 100); // Индикация отключения
        restart_scan_needed = true; // Установить флаг для перезапуска
        break;

    case 2: // INPUT_EVENT
        {
            // Парсинг HID данных
            esp_hidh_event_data_t *event_data = (esp_hidh_event_data_t *)param;
            if (event_data && event_data->input.data && event_data->input.length > 0) {
                // Логируем сырые данные для отладки
                Serial.printf("HID data (%d bytes): ", event_data->input.length);
                for (int i = 0; i < event_data->input.length; i++) {
                    Serial.printf("%02X ", event_data->input.data[i]);
                }
                Serial.println();

                // Обработка HID Consumer Control команд
                // BT13 отправляет данные в формате: [Usage Low] [Usage High]
                if (event_data->input.length >= 2) {
                    uint16_t usage = (event_data->input.data[1] << 8) | event_data->input.data[0];
                    uint32_t current_time = millis();

                    // Проверяем, что это нажатие (не отпускание)
                    bool pressed = (usage != 0);

                    if (pressed) {
                        Serial.printf("HID Usage: 0x%04X\n", usage);

                        // Обработка различных типов нажатий
                        switch (usage) {
                            case HID_USAGE_SHORT_PLUS: // 0x0004 - Короткое нажатие +
                                Serial.println("Command: Short + (increase level)");
                                short_press_plus();
                                break;

                            case HID_USAGE_SHORT_MINUS: // 0x0008 - Короткое нажатие -
                                Serial.println("Command: Short - (decrease level)");
                                short_press_minus();
                                break;

                            case HID_USAGE_STOP: // 0x0010 - Кнопка STOP
                                Serial.println("Command: STOP");
                                motor_stop_command();
                                break;

                            case HID_USAGE_LONG_PLUS: // 0x0001 - Длительное нажатие +
                                if (!is_long_press_detected || long_press_button != usage) {
                                    // Первое событие длительного нажатия +
                                    long_press_button = usage;
                                    long_press_start_time = current_time;
                                    is_long_press_detected = true;
                                    Serial.println("Command: Long + started (100% forward)");
                                    start_long_press_plus();
                                }
                                // Обновляем время последнего события длительного нажатия
                                last_long_press_event_time = current_time;
                                break;

                            case HID_USAGE_LONG_MINUS: // 0x0002 - Длительное нажатие -
                                if (!is_long_press_detected || long_press_button != usage) {
                                    // Первое событие длительного нажатия -
                                    long_press_button = usage;
                                    long_press_start_time = current_time;
                                    is_long_press_detected = true;
                                    Serial.println("Command: Long - started (100% backward)");
                                    start_long_press_minus();
                                }
                                // Обновляем время последнего события длительного нажатия
                                last_long_press_event_time = current_time;
                                break;

                            default:
                                Serial.printf("Unknown HID command: 0x%04X\n", usage);
                                break;
                        }
                    } else {
                        // Кнопка отпущена (usage == 0)
                        if (is_long_press_detected) {
                            // Проверяем, прошло ли достаточно времени с последнего события длительного нажатия
                            uint32_t time_since_last_event = current_time - last_long_press_event_time;
                            
                            if (time_since_last_event > LONG_PRESS_RELEASE_TIMEOUT_MS) {
                                // Реальное отпускание длительного нажатия
                                Serial.printf("Long press released (timeout: %lu ms)\n", time_since_last_event);
                                end_long_press();
                                
                                // Сброс состояния длительного нажатия
                                long_press_button = 0;
                                is_long_press_detected = false;
                                long_press_start_time = 0;
                                last_long_press_event_time = 0;
                            } else {
                                // Промежуточное событие отпускания - игнорируем
                                Serial.printf("Intermediate release ignored (time: %lu ms)\n", time_since_last_event);
                            }
                        } else {
                            Serial.println("Button released");
                        }
                    }
                }
            }

            led_blink(1, 50);
        }
        break;

    default:
        Serial.printf("HID Host event: %ld\n", event_id);
        break;
    }
}

// Задача мониторинга соединения
static void connection_monitor_task(void *pvParameters)
{
    Serial.println("Connection monitoring task started");

    while (1) {
        // Проверяем флаг перезапуска каждые 500мс
        vTaskDelay(pdMS_TO_TICKS(500));

        uint32_t current_time = millis();

        // Проверка состояния Bluetooth
        if (bluetooth_enabled) {
            esp_bt_controller_status_t controller_status = esp_bt_controller_get_status();
            esp_bluedroid_status_t bluedroid_status = esp_bluedroid_get_status();
            
            if (controller_status != ESP_BT_CONTROLLER_STATUS_ENABLED || 
                bluedroid_status != ESP_BLUEDROID_STATUS_ENABLED) {
                Serial.println("WARNING: Bluetooth state changed, marking as disabled");
                bluetooth_enabled = false;
            }
        }

        // Проверка автоматической остановки мотора при длительном отключении
        if (!bt13_connected && disconnection_start_time > 0) {
            uint32_t disconnection_duration = current_time - disconnection_start_time;

            if (disconnection_duration >= MOTOR_STOP_TIMEOUT_MS) {
                if (motor_enabled || speed_level != 0) {
                    Serial.printf("Motor stopped automatically: no connection for %lu seconds\n",
                             disconnection_duration / 1000);
                    motor_stop();
                    led_blink(10, 100); // Длинная индикация автоматической остановки
                }
                // Сбросить таймер, чтобы не повторять остановку
                disconnection_start_time = 0;
            }
        }

        if (restart_scan_needed) {
            Serial.println("Restart scan flag detected!");
            restart_scan_needed = false;

            Serial.println("Restarting BT13 scan in 3 seconds...");
            vTaskDelay(pdMS_TO_TICKS(3000)); // Wait 3 seconds before restart

            if (!bt13_connected) { // Check if still not connected
                Serial.println("Starting BT13 scan...");
                start_scan_for_bt13();
            } else {
                Serial.println("BT13 already connected, canceling restart");
            }
        }

        // Дополнительная проверка: если долго нет соединения, перезапускаем поиск
        static uint32_t last_connection_check = 0;

        if (!bt13_connected && bluetooth_enabled && (current_time - last_connection_check > 30000)) { // 30 seconds
            Serial.println("Long disconnection, restarting scan...");
            start_scan_for_bt13();
            last_connection_check = current_time;
        }

        if (bt13_connected) {
            last_connection_check = current_time;
        }
    }
}