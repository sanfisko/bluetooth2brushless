/*
 * Управление бесщеточным двигателем постоянного тока с ESP32 и Bluetooth-пультом
 * 
 * Этот проект демонстрирует управление бесщеточным двигателем с помощью ESP32
 * и трехкнопочного Bluetooth-пульта (BT13).
 * 
 * Автор: OpenHands
 * Дата: 2025-06-02
 * Лицензия: MIT
 */

#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

// Определение пинов
const int speedPin = 25;        // Пин PWM для управления скоростью
const int directionPin = 26;    // GPIO-пин для управления направлением
const int ledPin = 2;           // Встроенный LED для индикации состояния

// Настройки PWM
const int freq = 1000;          // Частота PWM (1кГц)
const int pwmChannel = 0;       // Канал PWM
const int resolution = 8;       // Разрешение 8 бит (0-255)

// Переменные управления двигателем
int speed = 0;                  // Скорость двигателя (0-255)
bool forward = true;            // Направление двигателя (true = вперед, false = назад)
bool motorEnabled = false;      // Состояние двигателя (включен/выключен)

// Настройки управления
const int speedStep = 10;       // Шаг изменения скорости
const int maxSpeed = 255;       // Максимальная скорость
const int minSpeed = 0;         // Минимальная скорость

// Переменные для отладки
unsigned long lastCommandTime = 0;
const unsigned long commandTimeout = 5000; // Таймаут команд (5 секунд)

void setup() {
  // Инициализация серийного порта для отладки
  Serial.begin(115200);
  Serial.println("=== ESP32 Motor Control System ===");
  Serial.println("Инициализация системы...");

  // Инициализация Bluetooth
  SerialBT.begin("ESP32_Motor_Control");  // Имя Bluetooth-устройства
  Serial.println("Bluetooth-устройство запущено: ESP32_Motor_Control");
  Serial.println("Ожидание сопряжения с пультом BT13...");

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

  // Начальное состояние
  updateMotorState();
  Serial.println("Система готова к работе!");
  Serial.println("Команды: '+' - увеличить скорость, '-' - уменьшить скорость, 'P' - изменить направление");
  Serial.println("=========================================");
}

void loop() {
  // Проверка наличия данных Bluetooth
  if (SerialBT.available()) {
    char command = SerialBT.read();
    lastCommandTime = millis();
    
    Serial.print("Получена команда: ");
    Serial.println(command);

    // Мигание LED при получении команды
    blinkLED(1, 100);

    // Обработка команд от пульта BT13
    processCommand(command);
  }

  // Проверка таймаута команд (автоматическая остановка)
  if (motorEnabled && (millis() - lastCommandTime > commandTimeout)) {
    Serial.println("Таймаут команд - остановка двигателя");
    stopMotor();
  }

  // Индикация состояния через LED
  updateLEDStatus();
  
  delay(20);
}

void processCommand(char command) {
  switch (command) {
    case '+':  // Увеличение громкости (увеличение скорости)
    case 'U':  // Альтернативная команда для увеличения
      increaseSpeed();
      break;

    case '-':  // Уменьшение громкости (уменьшение скорости)
    case 'D':  // Альтернативная команда для уменьшения
      decreaseSpeed();
      break;

    case 'P':  // Воспроизведение/Пауза (переключение направления)
    case 'p':  // Альтернативная команда
      toggleDirection();
      break;

    case 'S':  // Стоп
    case 's':
      stopMotor();
      break;

    case 'E':  // Включить/выключить двигатель
    case 'e':
      toggleMotor();
      break;

    default:
      Serial.println("Неизвестная команда: " + String(command));
      // Отправка списка доступных команд
      sendHelpMessage();
      break;
  }
}

void increaseSpeed() {
  if (!motorEnabled) {
    motorEnabled = true;
    Serial.println("Двигатель включен");
  }
  
  speed = min(speed + speedStep, maxSpeed);
  updateMotorState();
  
  Serial.print("Скорость увеличена до: ");
  Serial.print(speed);
  Serial.print("/");
  Serial.println(maxSpeed);
}

void decreaseSpeed() {
  speed = max(speed - speedStep, minSpeed);
  
  if (speed == 0) {
    motorEnabled = false;
    Serial.println("Двигатель остановлен (скорость = 0)");
  }
  
  updateMotorState();
  
  Serial.print("Скорость уменьшена до: ");
  Serial.print(speed);
  Serial.print("/");
  Serial.println(maxSpeed);
}

void toggleDirection() {
  forward = !forward;
  updateMotorState();
  
  Serial.print("Направление изменено на: ");
  Serial.println(forward ? "ВПЕРЕД" : "НАЗАД");
}

void stopMotor() {
  speed = 0;
  motorEnabled = false;
  updateMotorState();
  Serial.println("Двигатель остановлен");
}

void toggleMotor() {
  motorEnabled = !motorEnabled;
  if (!motorEnabled) {
    speed = 0;
  }
  updateMotorState();
  
  Serial.print("Двигатель ");
  Serial.println(motorEnabled ? "ВКЛЮЧЕН" : "ВЫКЛЮЧЕН");
}

void updateMotorState() {
  // Обновление PWM для скорости
  int actualSpeed = motorEnabled ? speed : 0;
  ledcWrite(pwmChannel, actualSpeed);
  
  // Обновление направления
  digitalWrite(directionPin, forward ? HIGH : LOW);
  
  // Отправка статуса по Bluetooth
  sendStatus();
}

void sendStatus() {
  String status = "Статус: ";
  status += motorEnabled ? "ВКЛ" : "ВЫКЛ";
  status += " | Скорость: " + String(speed) + "/" + String(maxSpeed);
  status += " | Направление: " + String(forward ? "ВПЕРЕД" : "НАЗАД");
  
  SerialBT.println(status);
  Serial.println(status);
}

void sendHelpMessage() {
  SerialBT.println("=== Команды управления ===");
  SerialBT.println("+ или U - Увеличить скорость");
  SerialBT.println("- или D - Уменьшить скорость");
  SerialBT.println("P или p - Изменить направление");
  SerialBT.println("S или s - Остановить двигатель");
  SerialBT.println("E или e - Включить/выключить двигатель");
  SerialBT.println("========================");
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
  
  if (motorEnabled && speed > 0) {
    // Быстрое мигание при работе двигателя
    if (millis() - lastBlink > 200) {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      lastBlink = millis();
    }
  } else if (motorEnabled) {
    // Медленное мигание при включенном, но остановленном двигателе
    if (millis() - lastBlink > 1000) {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      lastBlink = millis();
    }
  } else {
    // LED выключен при выключенном двигателе
    digitalWrite(ledPin, LOW);
  }
}