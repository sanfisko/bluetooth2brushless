# Исправление ошибок компиляции для ESP32 Arduino Core 3.2.0

## Проблема
Код `bluetooth2brushless_hid_client.ino` не компилировался с ESP32 Arduino Core версии 3.2.0 из-за изменений в API.

## Исправленные ошибки

### 1. Устаревшие функции PWM
**Было:**
```cpp
ledcSetup(pwmChannel, freq, resolution);
ledcAttachPin(speedPin, pwmChannel);
ledcWrite(pwmChannel, speed);
```

**Стало:**
```cpp
if (!ledcAttach(speedPin, freq, resolution)) {
  Serial.println("Ошибка инициализации PWM!");
  return;
}
ledcWrite(speedPin, currentSpeed);
```

### 2. Неопределенные переменные
Добавлены недостающие переменные:
```cpp
int currentSpeed = 0;           // Текущая скорость PWM (0-255)
bool currentDirection = true;   // Текущее направление (true = вперед)
```

### 3. Исправлены обращения к переменным
- Заменено `speed` на `currentSpeed`
- Заменено `forward` на `currentDirection`
- Удалена ссылка на неопределенную `longPressActive`

### 4. Обновлена функция updateMotorState()
- Использует новый API `ledcWrite(speedPin, currentSpeed)` вместо `ledcWrite(pwmChannel, actualSpeed)`
- Правильно управляет глобальными переменными состояния

## Совместимость
Код теперь совместим с:
- ESP32 Arduino Core 3.2.0
- BluetoothSerial библиотека версии 3.2.0

## Тестирование
После внесения изменений код должен компилироваться без ошибок для платы ESP32.