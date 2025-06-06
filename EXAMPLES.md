# 🎮 Примеры (как прокачать)

## 🎯 Базовое управление

### BT13 пульт
```
🔊+ = быстрее
🔉- = медленнее  
⏯️ = назад-вперед
```

### Телефон (Bluetooth Terminal)
```
+ = газ
- = тормоз
P = реверс
S = стоп
```
## 🚀 Прокачка

### 🌊 Плавная скорость
```cpp
void smoothSpeed(int target) {
  while (speed != target) {
    speed += (target > speed) ? 1 : -1;
    ledcWrite(pwmChannel, speed);
    delay(5);  // Плавность
  }
}
```

### 🎚️ Предустановки скорости
```cpp
int presets[] = {0, 50, 100, 150, 200, 255};

// Команды 1-6 = разные скорости
case '1': setSpeed(presets[0]); break;
case '2': setSpeed(presets[1]); break;
// и так далее
```

### 🚨 Кнопка СТОП
```cpp
const int stopPin = 4;

void setup() {
  pinMode(stopPin, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(stopPin) == LOW) {
    stopMotor();  // Аварийный стоп
  }
}
```

### 📊 Энкодер оборотов
```cpp
volatile int pulses = 0;

void IRAM_ATTR encoderISR() {
  pulses++;
}

void setup() {
  attachInterrupt(18, encoderISR, RISING);
}

// RPM = pulses * 60 / (время * импульсов_на_оборот)
```

### ⚡ Датчик тока
```cpp
float getCurrent() {
  int raw = analogRead(A0);
  return raw * 3.3 / 4095.0 / 0.1;  // Для ACS712
}

void loop() {
  float current = getCurrent();
  if (current > 5.0) {  // Защита от перегрузки
    stopMotor();
  }
}
```

### 🌡️ Температура
```cpp
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(5);
DallasTemperature sensors(&oneWire);

void checkTemperature() {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  
  if (temp > 80.0) {  // Перегрев
    stopMotor();
    Serial.println("ПЕРЕГРЕВ!");
  }
}
```

### 📱 WiFi управление
```cpp
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

void handleSpeed() {
  int newSpeed = server.arg("speed").toInt();
  setSpeed(newSpeed);
  server.send(200, "text/plain", "OK");
}

void setup() {
  WiFi.begin("SSID", "password");
  server.on("/speed", handleSpeed);
  server.begin();
}
```

## 🎯 Готовые проекты

### 🚗 RC машинка
- Два мотора (левый/правый)
- Сервопривод для поворота
- FPV камера

### 🚁 Дрон
- 4 мотора
- IMU датчик
- GPS модуль

### 🛥️ Катер
- Водонепроницаемый корпус
- Сервопривод руля
- Аварийное отключение

**Главное - не сожги дом!** 🔥