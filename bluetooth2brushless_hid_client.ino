/*
 * ESP32 HID Client для подключения к пульту BT13
 * Управление бесщеточным двигателем через HID команды
 * 
 * ВАЖНО: Этот код требует ESP32 Arduino Core версии 2.0.0 или выше
 * и библиотеки ESP32-BLE-Arduino
 * 
 * Автор: OpenHands
 * Дата: 2025-06-06
 */

#include "BluetoothSerial.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_err.h"

// Определение пинов
const int speedPin = 25;        // Пин PWM для управления скоростью
const int directionPin = 26;    // GPIO-пин для управления направлением
const int ledPin = 2;           // Встроенный LED для индикации состояния

// Настройки PWM
const int freq = 1000;          // Частота PWM (1кГц)
const int pwmChannel = 0;       // Канал PWM
const int resolution = 8;       // Разрешение 8 бит (0-255)

// Переменные управления двигателем
int speedLevel = 0;             // Уровень скорости от -10 до +10 (0 = стоп)
bool motorEnabled = false;      // Состояние двигателя (включен/выключен)

// Настройки управления
const int maxSpeedLevel = 10;   // Максимальный уровень скорости
const int speedLevels = 21;     // Всего уровней: -10..0..+10
const int pwmPerLevel = 25;     // PWM на уровень (255/10 ≈ 25)

// MAC адрес пульта BT13
uint8_t bt13_address[6] = {0x8B, 0xEB, 0x75, 0x4E, 0x65, 0x97};
bool bt13_connected = false;
bool scanning = false;

BluetoothSerial SerialBT;

// Функции управления двигателем
void shortPressPlus();          // Короткое нажатие +
void shortPressMinus();         // Короткое нажатие -
void longPressPlus();           // Длинное нажатие +
void longPressMinus();          // Длинное нажатие -
void stopMotor();               // Остановка мотора
void updateMotorState();        // Обновление состояния мотора
void handleButtonPress(uint8_t key, bool pressed); // Обработка нажатий
void checkLongPress();          // Проверка длинного нажатия
void blinkLED(int times, int delayMs);

// Bluetooth функции
void startScanForBT13();
void connectToBT13();
bool isBT13Device(esp_bd_addr_t address);

// Callback для GAP событий
void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

void setup() {
  // Инициализация серийного порта для отладки
  Serial.begin(115200);
  Serial.println("=== ESP32 HID Client Motor Control ===");
  Serial.println("Инициализация системы...");

  // Настройка PWM для управления скоростью
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(speedPin, pwmChannel);
  ledcWrite(pwmChannel, speed);
  Serial.println("PWM инициализирован на пине " + String(speedPin));

  // Настройка пина направления
  pinMode(directionPin, OUTPUT);
  digitalWrite(directionPin, forward ? HIGH : LOW);
  Serial.println("Пин направления инициализирован на пине " + String(directionPin));

  // Настройка индикаторного LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  Serial.println("LED индикатор инициализирован на пине " + String(ledPin));

  // Инициализация Bluetooth
  if (!SerialBT.begin("ESP32_HID_Client")) {
    Serial.println("Ошибка инициализации Bluetooth!");
    return;
  }
  
  // Регистрация callback для GAP событий
  esp_bt_gap_register_callback(gap_callback);
  
  Serial.println("Bluetooth инициализирован");
  Serial.print("Поиск пульта BT13 (MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", bt13_address[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println(")...");

  // Начальное состояние
  updateMotorState();
  
  // Начать поиск BT13
  startScanForBT13();
  
  Serial.println("Система готова к работе!");
  Serial.println("=========================================");
}

void loop() {
  // Проверка подключения к BT13
  if (!bt13_connected && !scanning) {
    Serial.println("Попытка переподключения к BT13...");
    startScanForBT13();
    delay(5000);
  }

  // BT13 сам определяет длинные нажатия - checkLongPress больше не нужна

  // Индикация состояния через LED
  updateLEDStatus();
  
  // Проверка входящих данных (если используется Serial режим)
  if (SerialBT.available()) {
    String data = SerialBT.readString();
    Serial.println("Получены данные: " + data);
    processHIDData(data);
  }
  
  delay(50); // Уменьшили задержку для более точной обработки длинных нажатий
}

void startScanForBT13() {
  if (scanning) return;
  
  scanning = true;
  Serial.println("Начинаем поиск устройств...");
  
  // Начать обнаружение устройств
  esp_err_t result = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
  if (result != ESP_OK) {
    Serial.println("Ошибка запуска поиска: " + String(result));
    scanning = false;
  }
}

void connectToBT13() {
  Serial.println("Попытка подключения к BT13...");
  
  // Попытка подключения через SerialBT
  if (SerialBT.connect(bt13_address)) {
    bt13_connected = true;
    Serial.println("Успешно подключен к BT13!");
    blinkLED(3, 200);
  } else {
    Serial.println("Ошибка подключения к BT13");
    bt13_connected = false;
  }
}

bool isBT13Device(esp_bd_addr_t address) {
  for (int i = 0; i < 6; i++) {
    if (address[i] != bt13_address[i]) {
      return false;
    }
  }
  return true;
}

void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT: {
      Serial.printf("Найдено устройство: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   param->disc_res.bda[0], param->disc_res.bda[1], param->disc_res.bda[2],
                   param->disc_res.bda[3], param->disc_res.bda[4], param->disc_res.bda[5]);
      
      if (isBT13Device(param->disc_res.bda)) {
        Serial.println("Найден BT13! Останавливаем поиск...");
        esp_bt_gap_cancel_discovery();
        scanning = false;
        
        // Копируем адрес и подключаемся
        memcpy(bt13_address, param->disc_res.bda, 6);
        connectToBT13();
      }
      break;
    }
    
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
      if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
        Serial.println("Поиск устройств завершен");
        scanning = false;
      } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
        Serial.println("Поиск устройств начат");
      }
      break;
    }
    
    default:
      break;
  }
}

void processHIDData(String data) {
  // Обработка HID команд с поддержкой коротких/длинных нажатий
  data.trim();
  
  if (data == "VOL_UP" || data == "+") {
    handleButtonPress(0xE9, true); // Volume Up нажата
  } else if (data == "VOL_DOWN" || data == "-") {
    handleButtonPress(0xEA, true); // Volume Down нажата
  } else if (data == "PLAY_PAUSE" || data == "P") {
    Serial.println("Команда: СТОП");
    stopMotor();
  } else if (data.startsWith("RELEASE_")) {
    // Обработка отпускания кнопок
    if (data == "RELEASE_VOL_UP") {
      handleButtonPress(0xE9, false);
    } else if (data == "RELEASE_VOL_DOWN") {
      handleButtonPress(0xEA, false);
    }
  } else {
    Serial.println("Неизвестная команда: " + data);
  }
}

// Обработка реальных HID событий от BT13
void processHIDEvent(uint16_t usage, bool pressed) {
  if (!pressed) return; // Обрабатываем только нажатия
  
  switch (usage) {
    case 0x00B5: // Next Song (короткое нажатие +)
      Serial.println("Короткое +: Увеличение уровня");
      shortPressPlus();
      break;
      
    case 0x00B6: // Previous Song (короткое нажатие -)
      Serial.println("Короткое -: Уменьшение уровня");
      shortPressMinus();
      break;
      
    case 0x00E9: // Volume Up (длинное нажатие +)
      Serial.println("Длинное +: Максимум вперед");
      longPressPlus();
      break;
      
    case 0x00EA: // Volume Down (длинное нажатие -)
      Serial.println("Длинное -: Максимум назад");
      longPressMinus();
      break;
      
    case 0x00CD: // Play/Pause (средняя кнопка)
      Serial.println("Средняя кнопка: СТОП");
      stopMotor();
      break;
      
    default:
      Serial.printf("Неизвестная HID команда: 0x%04X\n", usage);
      break;
  }
}

// Упрощенная обработка - BT13 сам различает короткие/длинные нажатия
void handleButtonPress(uint8_t key, bool pressed) {
  // Эта функция теперь используется только для совместимости
  // Основная обработка происходит в processHIDEvent()
  if (pressed) {
    Serial.printf("Устаревший вызов: кнопка 0x%02X нажата\n", key);
  } else {
    Serial.printf("Устаревший вызов: кнопка 0x%02X отпущена\n", key);
  }
}

void checkLongPress() {
  // Функция больше не нужна - BT13 сам определяет длинные нажатия
  // и отправляет соответствующие HID коды (VOLUME_UP/VOLUME_DOWN)
}

void shortPressPlus() {
  if (speedLevel < maxSpeedLevel) {
    speedLevel++;
    motorEnabled = true;
    updateMotorState();
    Serial.print("Короткое +: Уровень скорости = ");
    Serial.println(speedLevel);
  }
}

void shortPressMinus() {
  if (speedLevel > -maxSpeedLevel) {
    speedLevel--;
    if (speedLevel == 0) {
      motorEnabled = false;
    } else {
      motorEnabled = true;
    }
    updateMotorState();
    Serial.print("Короткое -: Уровень скорости = ");
    Serial.println(speedLevel);
  }
}

void longPressPlus() {
  speedLevel = maxSpeedLevel;
  motorEnabled = true;
  updateMotorState();
  Serial.println("Длинное +: Максимальная скорость вперед");
}

void longPressMinus() {
  speedLevel = -maxSpeedLevel;
  motorEnabled = true;
  updateMotorState();
  Serial.println("Длинное -: Максимальная скорость назад");
}

void stopMotor() {
  speedLevel = 0;
  motorEnabled = false;
  updateMotorState();
  Serial.println("Двигатель остановлен");
}

void updateMotorState() {
  // Вычисление PWM и направления на основе уровня скорости
  int actualSpeed = 0;
  bool forward = true;
  
  if (motorEnabled && speedLevel != 0) {
    actualSpeed = abs(speedLevel) * pwmPerLevel;
    forward = (speedLevel > 0);
  }
  
  // Обновление PWM для скорости
  ledcWrite(pwmChannel, actualSpeed);
  
  // Обновление направления
  digitalWrite(directionPin, forward ? HIGH : LOW);
  
  // Отправка статуса
  String status = "Статус: ";
  status += motorEnabled ? "ВКЛ" : "ВЫКЛ";
  status += " | Уровень: " + String(speedLevel) + "/" + String(maxSpeedLevel);
  status += " | PWM: " + String(actualSpeed) + "/255";
  status += " | Направление: " + String(forward ? "ВПЕРЕД" : "НАЗАД");
  
  if (longPressActive) {
    status += " | ДЛИННОЕ НАЖАТИЕ";
  }
  
  Serial.println(status);
}

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(ledPin, HIGH);
    delay(delayMs);
    digitalWrite(ledPin, LOW);
    delay(delayMs);
  }
}

void updateLEDStatus() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  if (bt13_connected && motorEnabled && speed > 0) {
    // Быстрое мигание при работе двигателя
    if (millis() - lastBlink > 200) {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      lastBlink = millis();
    }
  } else if (bt13_connected) {
    // Медленное мигание при подключении
    if (millis() - lastBlink > 1000) {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      lastBlink = millis();
    }
  } else {
    // LED выключен при отсутствии подключения
    digitalWrite(ledPin, LOW);
  }
}