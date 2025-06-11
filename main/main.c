/*
 * ESP32 HID Host для подключения к пульту BT13
 * Управление бесщеточным двигателем через HID команды
 *
 * Автор: OpenHands
 * Дата: 2025-06-06
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_hidh.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"

static const char *TAG = "BT13_MOTOR_CONTROL";

// Пины для управления двигателем
#define MOTOR_SPEED_PIN     GPIO_NUM_25
#define MOTOR_DIR_PIN       GPIO_NUM_26
#define LED_PIN             GPIO_NUM_2

// PWM настройки
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY      1000

// MAC адрес пульта BT13
static esp_bd_addr_t bt13_addr = {0x8B, 0xEB, 0x75, 0x4E, 0x65, 0x97};

// Переменные управления двигателем
static int speed_level = 0;        // Уровень скорости от -5 до +5 (0 = стоп)
static bool motor_enabled = false; // Состояние двигателя
static bool long_press_active = false; // Флаг длинного нажатия

// Настройки управления
static const int max_speed_level = 5;    // Максимальный уровень скорости
static const int pwm_per_level = 51;     // PWM на уровень (255/5 ≈ 51)

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
static void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
static void hid_host_cb(void *handler_args, const char *event_name, int32_t event_id, void *param);
static void start_scan_for_bt13(void);
static void connection_monitor_task(void *pvParameters);

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "=== ESP32 HID Host Motor Control ===");
    ESP_LOGI(TAG, "System initialization...");

    // Инициализация NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Инициализация двигателя
    motor_init();
    ESP_LOGI(TAG, "Motor initialized");

    // Инициализация таймера отключения (система стартует без соединения)
    disconnection_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // Инициализация Bluetooth
    // Используем только Classic BT (BLE отключен в конфигурации)

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ESP_LOGI(TAG, "Initializing BT controller...");
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller init error: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Enabling BT controller...");
    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller enable error: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Initializing Bluedroid...");
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init error: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Enabling Bluedroid...");
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable error: %s", esp_err_to_name(ret));
        return;
    }

    // Регистрация GAP callback
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(bt_gap_cb));

    // Инициализация HID Host
    ESP_ERROR_CHECK(esp_hidh_init(&(esp_hidh_config_t){
        .callback = hid_host_cb,
        .event_stack_size = 4096,
        .callback_arg = NULL,
    }));

    ESP_LOGI(TAG, "Bluetooth initialized");
    ESP_LOGI(TAG, "Searching for BT13 remote (MAC: %02X:%02X:%02X:%02X:%02X:%02X)...",
             bt13_addr[0], bt13_addr[1], bt13_addr[2],
             bt13_addr[3], bt13_addr[4], bt13_addr[5]);

    // Начать поиск BT13
    start_scan_for_bt13();

    // Создать задачу мониторинга соединения
    xTaskCreate(connection_monitor_task, "connection_monitor", 2048, NULL, 5, NULL);

    // Основной цикл
    while (1) {
        // Проверка таймаута длительного нажатия
        if (is_long_press_detected && last_long_press_event_time > 0) {
            uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            uint32_t time_since_last_event = current_time - last_long_press_event_time;
            
            if (time_since_last_event > LONG_PRESS_RELEASE_TIMEOUT_MS) {
                // Таймаут - считаем что кнопка отпущена
                ESP_LOGI(TAG, "Long press timeout detected (%lu ms)", time_since_last_event);
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

        vTaskDelay(pdMS_TO_TICKS(50)); // Уменьшили задержку для более точной обработки
    }
}

static void motor_init(void)
{
    // Настройка PWM для скорости
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_CHANNEL,
        .duty       = 0,
        .gpio_num   = MOTOR_SPEED_PIN,
        .speed_mode = LEDC_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // Настройка пина направления
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << MOTOR_DIR_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    // Настройка LED
    io_conf.pin_bit_mask = (1ULL << LED_PIN);
    gpio_config(&io_conf);

    // Начальное состояние
    motor_update_state();
    gpio_set_level(LED_PIN, 0);
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
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, actual_speed));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

    // Обновление направления
    gpio_set_level(MOTOR_DIR_PIN, forward ? 1 : 0);

    ESP_LOGI(TAG, "State: %s | Level: %d/%d | PWM: %d/255 | Direction: %s%s",
             motor_enabled ? "ON" : "OFF",
             speed_level, max_speed_level,
             actual_speed,
             forward ? "FORWARD" : "BACKWARD",
             long_press_active ? " | LONG PRESS" : "");
}

static void print_motor_status(void)
{
    if (!motor_enabled || speed_level == 0) {
        ESP_LOGI(TAG, "Stopped");
    } else {
        int percentage = (abs(speed_level) * 100) / max_speed_level;
        const char* direction = (speed_level > 0) ? "forward" : "backward";
        ESP_LOGI(TAG, "Running %s at %d%%", direction, percentage);
    }
}

// Короткое нажатие + : добавляет 20% к скорости
static void short_press_plus(void)
{
    if (speed_level < max_speed_level) {
        speed_level++;
        motor_enabled = (speed_level != 0);
        motor_update_state();
        int percentage = (speed_level * 100) / max_speed_level;
        ESP_LOGI(TAG, "Short +: Speed level = %d (%d%% forward)", speed_level, percentage);
        print_motor_status();
    } else {
        ESP_LOGI(TAG, "Short +: Already at maximum forward speed");
    }
}

// Короткое нажатие - : убавляет 20% от скорости, может переключить направление
static void short_press_minus(void)
{
    if (speed_level > -max_speed_level) {
        speed_level--;
        motor_enabled = (speed_level != 0);
        motor_update_state();
        
        if (speed_level == 0) {
            ESP_LOGI(TAG, "Short -: Motor stopped");
        } else {
            int percentage = (abs(speed_level) * 100) / max_speed_level;
            const char* direction = (speed_level > 0) ? "forward" : "backward";
            ESP_LOGI(TAG, "Short -: Speed level = %d (%d%% %s)", speed_level, percentage, direction);
        }
        print_motor_status();
    } else {
        ESP_LOGI(TAG, "Short -: Already at maximum backward speed");
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
        ESP_LOGI(TAG, "Long + started: 100%% forward speed");
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
        ESP_LOGI(TAG, "Long - started: 100%% backward speed");
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
        ESP_LOGI(TAG, "Long press released: Full stop");
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
    ESP_LOGI(TAG, "STOP button: Motor stopped");
    print_motor_status();
}

static void motor_stop(void)
{
    speed_level = 0;
    motor_enabled = false;
    long_press_active = false;
    motor_update_state();
    ESP_LOGI(TAG, "Motor stopped");
}

static void led_blink(int times, int delay_ms)
{
    for (int i = 0; i < times; i++) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

// Unused legacy function - commented out to avoid warnings
/*
static void handle_hid_event(uint16_t usage, bool pressed)
{
    if (!pressed) return; // Обрабатываем только нажатия

    switch (usage) {
        case 0x00B5: // Next Song (короткое нажатие +)
            ESP_LOGI(TAG, "Короткое +: Увеличение уровня");
            short_press_plus();
            break;

        case 0x00B6: // Previous Song (короткое нажатие -)
            ESP_LOGI(TAG, "Короткое -: Уменьшение уровня");
            short_press_minus();
            break;

        case 0x00E9: // Volume Up (длинное нажатие +)
            ESP_LOGI(TAG, "Длинное +: Максимум вперед");
            long_press_plus();
            break;

        case 0x00EA: // Volume Down (длинное нажатие -)
            ESP_LOGI(TAG, "Длинное -: Максимум назад");
            long_press_minus();
            break;

        case 0x00CD: // Play/Pause (средняя кнопка)
            ESP_LOGI(TAG, "Средняя кнопка: СТОП");
            motor_stop();
            break;

        default:
            ESP_LOGI(TAG, "Неизвестная HID команда: 0x%04X", usage);
            break;
    }
}
*/

// Unused legacy function - commented out to avoid warnings
/*
static void handle_button_press(uint8_t key, bool pressed)
{
    if (pressed) {
        // Кнопка нажата
        button_pressed = true;
        current_pressed_button = key;
        press_start_time = esp_timer_get_time();

        if (key == 0xE9) { // Volume Up
            ESP_LOGI(TAG, "Volume+ нажата");
        } else if (key == 0xEA) { // Volume Down
            ESP_LOGI(TAG, "Volume- нажата");
        } else if (key == 0xCD) { // Play/Pause
            ESP_LOGI(TAG, "Команда: СТОП");
            motor_stop();
        }
    } else {
        // Кнопка отпущена
        if (button_pressed && current_pressed_button == key) {
            uint64_t press_duration = esp_timer_get_time() - press_start_time;
            button_pressed = false;
            current_pressed_button = 0;

            if (long_press_active) {
                // Завершение длинного нажатия - остановка
                long_press_active = false;
                motor_stop();
                ESP_LOGI(TAG, "Длинное нажатие завершено - остановка");
            } else if (press_duration < long_press_threshold) {
                // Короткое нажатие
                if (key == 0xE9) { // Volume Up
                    short_press_plus();
                } else if (key == 0xEA) { // Volume Down
                    short_press_minus();
                }
            }
        }
    }
}
*/



static void start_scan_for_bt13(void)
{
    if (scanning_in_progress) {
        ESP_LOGI(TAG, "Scan already in progress, skipping...");
        return;
    }

    if (bt13_connected) {
        ESP_LOGI(TAG, "BT13 already connected, scan not needed");
        return;
    }

    ESP_LOGI(TAG, "Searching for BT13 remote (MAC: %02X:%02X:%02X:%02X:%02X:%02X)...",
             bt13_addr[0], bt13_addr[1], bt13_addr[2],
             bt13_addr[3], bt13_addr[4], bt13_addr[5]);

    scanning_in_progress = true;
    esp_err_t ret = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error starting discovery: %s", esp_err_to_name(ret));
        scanning_in_progress = false;
    }
}

static void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT: {
        ESP_LOGI(TAG, "Found device: %02x:%02x:%02x:%02x:%02x:%02x",
                 param->disc_res.bda[0], param->disc_res.bda[1], param->disc_res.bda[2],
                 param->disc_res.bda[3], param->disc_res.bda[4], param->disc_res.bda[5]);

        // Проверяем, это ли наш BT13
        if (memcmp(param->disc_res.bda, bt13_addr, ESP_BD_ADDR_LEN) == 0) {
            ESP_LOGI(TAG, "Found BT13! Stopping discovery...");
            esp_bt_gap_cancel_discovery();

            // Подключаемся к BT13
            ESP_LOGI(TAG, "Connecting to BT13...");
            esp_hidh_dev_t *dev = esp_hidh_dev_open(param->disc_res.bda, ESP_HID_TRANSPORT_BT, 0);
            if (dev == NULL) {
                ESP_LOGE(TAG, "Failed to connect to BT13");
            }
        }
        break;
    }
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            ESP_LOGI(TAG, "Device discovery completed");
            scanning_in_progress = false;
        } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
            ESP_LOGI(TAG, "Device discovery started");
            scanning_in_progress = true;
        }
        break;
    default:
        break;
    }
}

static void hid_host_cb(void *handler_args, const char *event_name, int32_t event_id, void *param)
{
    ESP_LOGI(TAG, "HID Host event: %s (ID: %ld)", event_name ? event_name : "unknown", event_id);

    // В ESP-IDF v5.4.1 изменился API для HID Host
    // Используем event_id для определения типа события
    switch (event_id) {
    case 0: // OPEN_EVENT
        ESP_LOGI(TAG, "BT13 connected successfully!");
        bt13_connected = true;
        disconnection_start_time = 0; // Reset disconnection timer
        ESP_LOGI(TAG, "Ready to receive commands from remote");
        led_blink(3, 200);
        break;

    case 1: // CLOSE_EVENT
    case 4: // CLOSE_EVENT/DISCONNECT_EVENT (альтернативный ID)
        bt13_connected = false;
        disconnection_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS; // Запомнить время отключения
        ESP_LOGI(TAG, "BT13 disconnected. Restart scan scheduled...");
        motor_stop(); // Остановить двигатель при отключении
        led_blink(5, 100); // Индикация отключения
        restart_scan_needed = true; // Установить флаг для перезапуска
        break;

    case 2: // INPUT_EVENT
        // Парсинг HID данных
        esp_hidh_event_data_t *event_data = (esp_hidh_event_data_t *)param;
        if (event_data && event_data->input.data && event_data->input.length > 0) {
            // Логируем сырые данные для отладки
            ESP_LOGI(TAG, "HID data (%d bytes):", event_data->input.length);
            for (int i = 0; i < event_data->input.length; i++) {
                printf("%02X ", event_data->input.data[i]);
            }
            printf("\n");

            // Обработка HID Consumer Control команд
            // BT13 отправляет данные в формате: [Usage Low] [Usage High]
            if (event_data->input.length >= 2) {
                uint16_t usage = (event_data->input.data[1] << 8) | event_data->input.data[0];
                uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

                // Проверяем, что это нажатие (не отпускание)
                bool pressed = (usage != 0);

                if (pressed) {
                    ESP_LOGI(TAG, "HID Usage: 0x%04X", usage);

                    // Обработка различных типов нажатий
                    switch (usage) {
                        case HID_USAGE_SHORT_PLUS: // 0x0004 - Короткое нажатие +
                            ESP_LOGI(TAG, "Command: Short + (increase level)");
                            short_press_plus();
                            break;

                        case HID_USAGE_SHORT_MINUS: // 0x0008 - Короткое нажатие -
                            ESP_LOGI(TAG, "Command: Short - (decrease level)");
                            short_press_minus();
                            break;

                        case HID_USAGE_STOP: // 0x0010 - Кнопка STOP
                            ESP_LOGI(TAG, "Command: STOP");
                            motor_stop_command();
                            break;

                        case HID_USAGE_LONG_PLUS: // 0x0001 - Длительное нажатие +
                            if (!is_long_press_detected || long_press_button != usage) {
                                // Первое событие длительного нажатия +
                                long_press_button = usage;
                                long_press_start_time = current_time;
                                is_long_press_detected = true;
                                ESP_LOGI(TAG, "Command: Long + started (100%% forward)");
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
                                ESP_LOGI(TAG, "Command: Long - started (100%% backward)");
                                start_long_press_minus();
                            }
                            // Обновляем время последнего события длительного нажатия
                            last_long_press_event_time = current_time;
                            break;

                        default:
                            ESP_LOGI(TAG, "Unknown HID command: 0x%04X", usage);
                            break;
                    }
                } else {
                    // Кнопка отпущена (usage == 0)
                    if (is_long_press_detected) {
                        // Проверяем, прошло ли достаточно времени с последнего события длительного нажатия
                        uint32_t time_since_last_event = current_time - last_long_press_event_time;
                        
                        if (time_since_last_event > LONG_PRESS_RELEASE_TIMEOUT_MS) {
                            // Реальное отпускание длительного нажатия
                            ESP_LOGI(TAG, "Long press released (timeout: %lu ms)", time_since_last_event);
                            end_long_press();
                            
                            // Сброс состояния длительного нажатия
                            long_press_button = 0;
                            is_long_press_detected = false;
                            long_press_start_time = 0;
                            last_long_press_event_time = 0;
                        } else {
                            // Промежуточное событие отпускания - игнорируем
                            ESP_LOGI(TAG, "Intermediate release ignored (time: %lu ms)", time_since_last_event);
                        }
                    } else {
                        ESP_LOGI(TAG, "Button released");
                    }
                }
            }
        }

        led_blink(1, 50);
        break;

    default:
        ESP_LOGI(TAG, "HID Host event: %ld", event_id);
        break;
    }
}

// Задача мониторинга соединения
static void connection_monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Connection monitoring task started");

    while (1) {
        // Проверяем флаг перезапуска каждые 500мс
        vTaskDelay(pdMS_TO_TICKS(500));

        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // Проверка автоматической остановки мотора при длительном отключении
        if (!bt13_connected && disconnection_start_time > 0) {
            uint32_t disconnection_duration = current_time - disconnection_start_time;

            if (disconnection_duration >= MOTOR_STOP_TIMEOUT_MS) {
                if (motor_enabled || speed_level != 0) {
                    ESP_LOGW(TAG, "Motor stopped automatically: no connection for %lu seconds",
                             disconnection_duration / 1000);
                    motor_stop();
                    led_blink(10, 100); // Длинная индикация автоматической остановки
                }
                // Сбросить таймер, чтобы не повторять остановку
                disconnection_start_time = 0;
            }
        }

        if (restart_scan_needed) {
            ESP_LOGI(TAG, "Restart scan flag detected!");
            restart_scan_needed = false;

            ESP_LOGI(TAG, "Restarting BT13 scan in 3 seconds...");
            vTaskDelay(pdMS_TO_TICKS(3000)); // Wait 3 seconds before restart

            if (!bt13_connected) { // Check if still not connected
                ESP_LOGI(TAG, "Starting BT13 scan...");
                start_scan_for_bt13();
            } else {
                ESP_LOGI(TAG, "BT13 already connected, canceling restart");
            }
        }

        // Дополнительная проверка: если долго нет соединения, перезапускаем поиск
        static uint32_t last_connection_check = 0;

        if (!bt13_connected && (current_time - last_connection_check > 30000)) { // 30 seconds
            ESP_LOGI(TAG, "Long disconnection, restarting scan...");
            start_scan_for_bt13();
            last_connection_check = current_time;
        }

        if (bt13_connected) {
            last_connection_check = current_time;
        }
    }
}