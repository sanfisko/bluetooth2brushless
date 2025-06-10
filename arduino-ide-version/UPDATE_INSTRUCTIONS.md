# ОБНОВЛЕНИЕ КОДА - ИСПРАВЛЕНИЕ ОШИБКИ КОМПИЛЯЦИИ

## ПРОБЛЕМА
Если вы получаете ошибку:
```
taking address of rvalue [-fpermissive]
```

Это означает, что у вас старая версия кода. Ошибка уже исправлена в репозитории.

## РЕШЕНИЕ

### 1. СКАЧАЙТЕ ОБНОВЛЕННЫЙ КОД
Перейдите на GitHub: https://github.com/dmoefasc/bluetooth2brushless

Нажмите зеленую кнопку **"Code"** → **"Download ZIP"**

### 2. ЗАМЕНИТЕ ФАЙЛ
1. Распакуйте скачанный архив
2. Найдите файл: `arduino-ide-version/bluetooth2brushless.ino`
3. Замените ваш старый файл на новый

### 3. ПРОВЕРЬТЕ ИСПРАВЛЕНИЕ
В новой версии строки 149-154 должны выглядеть так:
```cpp
// Инициализация HID Host
esp_hidh_config_t hidh_config = {
    .callback = hid_host_cb,
    .event_stack_size = 4096,
    .callback_arg = NULL,
};
ESP_ERROR_CHECK(esp_hidh_init(&hidh_config));
```

**НЕ ТАК (старая версия):**
```cpp
ESP_ERROR_CHECK(esp_hidh_init(&(esp_hidh_config_t){
    .callback = hid_host_cb,
    .event_stack_size = 4096,
    .callback_arg = NULL,
}));
```

### 4. КОМПИЛИРУЙТЕ
После замены файла код должен компилироваться без ошибок.

## АЛЬТЕРНАТИВНЫЙ СПОСОБ (GIT)
Если у вас установлен git:
```bash
cd /path/to/your/project
git pull origin main
```

## ПРОВЕРКА ВЕРСИИ
В начале файла должно быть:
```cpp
/*
 * ESP32 HID Host для подключения к пульту BT13
 * Управление бесщеточным двигателем через HID команды
 *
 * ВАЖНО: Этот код требует ESP32 Arduino Core версии 3.0.0 или выше
 * Основан на рабочей версии ESP-IDF с упрощенной инициализацией Bluetooth
 * Совместим с библиотекой 3.2 из https://espressif.github.io/arduino-esp32/package_esp32_index.json
 *
 * Автор: OpenHands
 * Дата: 2025-06-10
 */
```

Если у вас другой текст в комментарии - значит у вас старая версия.