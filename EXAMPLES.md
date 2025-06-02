# Примеры использования и расширения

## Базовое использование

### Управление через Bluetooth-терминал
```
Команды:
+ или U - Увеличить скорость на 10 единиц
- или D - Уменьшить скорость на 10 единиц  
P или p - Изменить направление (вперед/назад)
S или s - Остановить двигатель
E или e - Включить/выключить двигатель
```

### Пример сессии управления
```
> E          // Включить двигатель
Двигатель ВКЛЮЧЕН

> +          // Увеличить скорость
Скорость увеличена до: 10/255

> +          // Еще увеличить
Скорость увеличена до: 20/255

> P          // Изменить направление
Направление изменено на: НАЗАД

> S          // Остановить
Двигатель остановлен
```

## Расширенные возможности

### 1. Добавление плавного изменения скорости

Замените функции `increaseSpeed()` и `decreaseSpeed()` на:

```cpp
void smoothSpeedChange(int targetSpeed) {
  int currentSpeed = speed;
  int step = (targetSpeed > currentSpeed) ? 1 : -1;
  
  while (currentSpeed != targetSpeed) {
    currentSpeed += step;
    ledcWrite(pwmChannel, currentSpeed);
    delay(10); // Задержка для плавности
  }
  speed = targetSpeed;
}

void increaseSpeed() {
  if (!motorEnabled) {
    motorEnabled = true;
    Serial.println("Двигатель включен");
  }
  
  int newSpeed = min(speed + speedStep, maxSpeed);
  smoothSpeedChange(newSpeed);
  
  Serial.print("Скорость плавно увеличена до: ");
  Serial.println(speed);
}
```

### 2. Добавление предустановленных скоростей

```cpp
// Добавьте в начало файла
const int presetSpeeds[] = {0, 50, 100, 150, 200, 255};
const int numPresets = sizeof(presetSpeeds) / sizeof(presetSpeeds[0]);
int currentPreset = 0;

// Добавьте в processCommand()
case '1':
case '2':
case '3':
case '4':
case '5':
case '6':
  setPresetSpeed(command - '1');
  break;

// Новая функция
void setPresetSpeed(int preset) {
  if (preset >= 0 && preset < numPresets) {
    currentPreset = preset;
    speed = presetSpeeds[preset];
    motorEnabled = (speed > 0);
    updateMotorState();
    
    Serial.print("Установлена предустановленная скорость ");
    Serial.print(preset + 1);
    Serial.print(": ");
    Serial.println(speed);
  }
}
```

### 3. Добавление энкодера для обратной связи

```cpp
// Добавьте пины для энкодера
const int encoderPinA = 18;
const int encoderPinB = 19;

volatile long encoderPosition = 0;
volatile int lastEncoded = 0;

void setup() {
  // ... существующий код ...
  
  // Настройка энкодера
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(encoderPinA), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPinB), updateEncoder, CHANGE);
}

void updateEncoder() {
  int MSB = digitalRead(encoderPinA);
  int LSB = digitalRead(encoderPinB);
  
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;
  
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    encoderPosition++;
  }
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    encoderPosition--;
  }
  
  lastEncoded = encoded;
}

// Добавьте в loop() для отправки позиции
void sendEncoderData() {
  static unsigned long lastEncoderSend = 0;
  if (millis() - lastEncoderSend > 1000) { // Каждую секунду
    SerialBT.print("Позиция энкодера: ");
    SerialBT.println(encoderPosition);
    lastEncoderSend = millis();
  }
}
```

### 4. Добавление датчика тока

```cpp
const int currentSensorPin = A0;
const float currentSensitivity = 0.1; // В/А (зависит от датчика)

float readMotorCurrent() {
  int sensorValue = analogRead(currentSensorPin);
  float voltage = sensorValue * (3.3 / 4095.0); // ESP32 ADC
  float current = voltage / currentSensitivity;
  return current;
}

// Добавьте в loop()
void monitorCurrent() {
  static unsigned long lastCurrentCheck = 0;
  if (millis() - lastCurrentCheck > 500) { // Каждые 500мс
    float current = readMotorCurrent();
    
    if (current > 10.0) { // Защита от перегрузки
      Serial.println("ПЕРЕГРУЗКА! Остановка двигателя");
      stopMotor();
    }
    
    lastCurrentCheck = millis();
  }
}
```

### 5. Web-интерфейс управления

```cpp
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "YourWiFiName";
const char* password = "YourWiFiPassword";

WebServer server(80);

void setupWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Подключение к WiFi...");
  }
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/speed", handleSpeed);
  server.on("/direction", handleDirection);
  server.on("/stop", handleStop);
  server.begin();
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Motor Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; margin: 50px; }
        button { padding: 20px; margin: 10px; font-size: 18px; }
        .speed-btn { background-color: #4CAF50; color: white; }
        .dir-btn { background-color: #2196F3; color: white; }
        .stop-btn { background-color: #f44336; color: white; }
    </style>
</head>
<body>
    <h1>Управление двигателем ESP32</h1>
    <p>Текущая скорость: <span id="speed">)" + String(speed) + R"(</span></p>
    <p>Направление: <span id="direction">)" + String(forward ? "Вперед" : "Назад") + R"(</span></p>
    
    <button class="speed-btn" onclick="changeSpeed(1)">Увеличить скорость</button>
    <button class="speed-btn" onclick="changeSpeed(-1)">Уменьшить скорость</button><br>
    
    <button class="dir-btn" onclick="toggleDirection()">Изменить направление</button><br>
    
    <button class="stop-btn" onclick="stopMotor()">СТОП</button>
    
    <script>
        function changeSpeed(delta) {
            fetch('/speed?delta=' + delta).then(() => location.reload());
        }
        function toggleDirection() {
            fetch('/direction').then(() => location.reload());
        }
        function stopMotor() {
            fetch('/stop').then(() => location.reload());
        }
    </script>
</body>
</html>
  )";
  server.send(200, "text/html", html);
}

void handleSpeed() {
  if (server.hasArg("delta")) {
    int delta = server.arg("delta").toInt();
    if (delta > 0) {
      increaseSpeed();
    } else {
      decreaseSpeed();
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleDirection() {
  toggleDirection();
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  stopMotor();
  server.send(200, "text/plain", "OK");
}

// Добавьте в loop()
void loop() {
  server.handleClient();
  // ... остальной код ...
}
```

## Применения проекта

### 1. Робототехника
- Управление колесами робота
- Поворотные механизмы
- Подъемные устройства

### 2. Автоматизация
- Конвейерные ленты
- Вентиляторы с регулируемой скоростью
- Насосы

### 3. Хобби-проекты
- RC модели
- Дроны (для поворотных камер)
- Станки с ЧПУ (одна ось)

### 4. Образование
- Изучение PWM
- Основы управления двигателями
- Bluetooth-коммуникация

## Советы по оптимизации

### Производительность
- Используйте прерывания для критичных по времени операций
- Минимизируйте использование `delay()` в основном цикле
- Кэшируйте часто используемые значения

### Энергопотребление
- Используйте режим сна ESP32 при неактивности
- Отключайте Bluetooth при работе только с проводным управлением
- Оптимизируйте частоту PWM

### Надежность
- Добавьте watchdog timer
- Реализуйте проверку контрольных сумм для команд
- Используйте аппаратные прерывания для аварийной остановки