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
#include "esp_hid_gap.h"
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
static int speed_level = 0;        // Уровень скорости от -10 до +10 (0 = стоп)
static bool motor_enabled = false; // Состояние двигателя
static bool long_press_active = false; // Флаг длинного нажатия

// Настройки управления
static const int max_speed_level = 10;   // Максимальный уровень скорости
static const int pwm_per_level = 25;     // PWM на уровень (255/10 ≈ 25)

// Таймеры для определения длинного нажатия
static uint64_t press_start_time = 0;
static const uint64_t long_press_threshold = 500000; // 500мс в микросекундах
static uint8_t current_pressed_button = 0;
static bool button_pressed = false;

// HID Host переменные
static esp_hidh_dev_t *hid_dev = NULL;
static bool bt13_connected = false;

// Функции управления двигателем
static void motor_init(void);
static void motor_update_state(void);
static void motor_update_speed(void);
static void motor_update_direction(void);
static void motor_increase_speed(void);
static void motor_decrease_speed(void);
static void motor_toggle_direction(void);

// Функции обработки кнопок
static void handle_button_press(uint8_t key_code, bool pressed);
static void check_long_press(void);
static void motor_stop(void);
static void led_blink(int times, int delay_ms);

// Bluetooth функции
static void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
static void hid_host_cb(void *handler_args, esp_event_base_t base, int32_t id, void *event_data);
static void start_scan_for_bt13(void);

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "=== ESP32 HID Host Motor Control ===");
    ESP_LOGI(TAG, "Инициализация системы...");

    // Инициализация NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Инициализация двигателя
    motor_init();
    ESP_LOGI(TAG, "Двигатель инициализирован");

    // Инициализация Bluetooth
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // Регистрация GAP callback
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(bt_gap_cb));

    // Инициализация HID Host
    ESP_ERROR_CHECK(esp_hid_gap_init());
    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HIDH_EVENTS, ESP_EVENT_ANY_ID, 
                                               hid_host_cb, NULL));

    ESP_LOGI(TAG, "Bluetooth инициализирован");
    ESP_LOGI(TAG, "Поиск пульта BT13 (MAC: %02X:%02X:%02X:%02X:%02X:%02X)...", 
             bt13_addr[0], bt13_addr[1], bt13_addr[2], 
             bt13_addr[3], bt13_addr[4], bt13_addr[5]);

    // Начать поиск BT13
    start_scan_for_bt13();

    // Основной цикл
    while (1) {
        // Обработка длинных нажатий
        check_long_press();
        
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
    
    ESP_LOGI(TAG, "Состояние: %s | Уровень: %d/%d | PWM: %d/255 | Направление: %s%s", 
             motor_enabled ? "ВКЛ" : "ВЫКЛ",
             speed_level, max_speed_level,
             actual_speed,
             forward ? "ВПЕРЕД" : "НАЗАД",
             long_press_active ? " | ДЛИННОЕ НАЖАТИЕ" : "");
}

static void short_press_plus(void)
{
    if (speed_level < max_speed_level) {
        speed_level++;
        motor_enabled = true;
        motor_update_state();
        ESP_LOGI(TAG, "Короткое +: Уровень скорости = %d", speed_level);
    }
}

static void short_press_minus(void)
{
    if (speed_level > -max_speed_level) {
        speed_level--;
        if (speed_level == 0) {
            motor_enabled = false;
        } else {
            motor_enabled = true;
        }
        motor_update_state();
        ESP_LOGI(TAG, "Короткое -: Уровень скорости = %d", speed_level);
    }
}

static void long_press_plus(void)
{
    speed_level = max_speed_level;
    motor_enabled = true;
    long_press_active = true;
    motor_update_state();
    ESP_LOGI(TAG, "Длинное +: Максимальная скорость вперед");
}

static void long_press_minus(void)
{
    speed_level = -max_speed_level;
    motor_enabled = true;
    long_press_active = true;
    motor_update_state();
    ESP_LOGI(TAG, "Длинное -: Максимальная скорость назад");
}

static void motor_stop(void)
{
    speed_level = 0;
    motor_enabled = false;
    long_press_active = false;
    motor_update_state();
    ESP_LOGI(TAG, "Двигатель остановлен");
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

static void check_long_press(void)
{
    // Проверка длинного нажатия
    if (button_pressed && !long_press_active && 
        (esp_timer_get_time() - press_start_time) >= long_press_threshold) {
        
        long_press_active = true;
        
        if (current_pressed_button == 0xE9) { // Volume Up
            long_press_plus();
        } else if (current_pressed_button == 0xEA) { // Volume Down
            long_press_minus();
        }
    }
}

static void start_scan_for_bt13(void)
{
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
}

static void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT: {
        esp_bt_gap_dev_prop_t *p;
        ESP_LOGI(TAG, "Найдено устройство: %02x:%02x:%02x:%02x:%02x:%02x",
                 param->disc_res.bda[0], param->disc_res.bda[1], param->disc_res.bda[2],
                 param->disc_res.bda[3], param->disc_res.bda[4], param->disc_res.bda[5]);

        // Проверяем, это ли наш BT13
        if (memcmp(param->disc_res.bda, bt13_addr, ESP_BD_ADDR_LEN) == 0) {
            ESP_LOGI(TAG, "Найден BT13! Останавливаем поиск...");
            esp_bt_gap_cancel_discovery();
            
            // Подключаемся к BT13
            ESP_LOGI(TAG, "Подключение к BT13...");
            esp_hidh_dev_open(param->disc_res.bda, ESP_HID_TRANSPORT_BT, param->disc_res.bda);
        }
        break;
    }
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            ESP_LOGI(TAG, "Поиск устройств завершен");
        } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
            ESP_LOGI(TAG, "Поиск устройств начат");
        }
        break;
    default:
        break;
    }
}

static void hid_host_cb(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidh_event_data_t *data = (esp_hidh_event_data_t *)event_data;

    switch (id) {
    case ESP_HIDH_OPEN_EVENT:
        if (data->open.status == ESP_OK) {
            hid_dev = data->open.dev;
            bt13_connected = true;
            ESP_LOGI(TAG, "BT13 подключен успешно!");
            ESP_LOGI(TAG, "Готов к приему команд от пульта");
            led_blink(3, 200);
        } else {
            ESP_LOGE(TAG, "Ошибка подключения к BT13: %d", data->open.status);
            // Повторить поиск через 5 секунд
            vTaskDelay(pdMS_TO_TICKS(5000));
            start_scan_for_bt13();
        }
        break;

    case ESP_HIDH_CLOSE_EVENT:
        hid_dev = NULL;
        bt13_connected = false;
        ESP_LOGI(TAG, "BT13 отключен. Перезапуск поиска...");
        motor_stop(); // Остановить двигатель при отключении
        vTaskDelay(pdMS_TO_TICKS(2000));
        start_scan_for_bt13();
        break;

    case ESP_HIDH_INPUT_EVENT: {
        // Обработка HID событий от BT13
        if (data->input.length >= 2) {
            uint8_t key_code = data->input.data[1];
            bool key_pressed = (data->input.data[0] != 0);

            // Обработка HID Consumer Control событий
            if (data->input.length >= 3) {
                uint16_t usage = (data->input.data[2] << 8) | data->input.data[1];
                handle_hid_event(usage, key_pressed);
            } else {
                handle_button_press(key_code, key_pressed);
            }
            
            // Мигание LED при получении команды
            if (key_pressed) {
                led_blink(1, 50);
            }
        }
        break;
    }

    default:
        ESP_LOGI(TAG, "HID Host событие: %ld", id);
        break;
    }
}