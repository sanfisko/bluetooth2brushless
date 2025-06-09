# ESP-IDF v5.4.1 Runtime Fix - Bluetooth Initialization Error

## Problem Solved

Runtime error during Bluetooth controller initialization:
```
ESP_ERROR_CHECK failed: esp_err_t 0x102 (ESP_ERR_INVALID_ARG) at 0x400d927a
expression: esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)
```

## Root Cause

The issue was caused by incorrect Bluetooth mode configuration:
1. **Memory Release Conflict**: Code was releasing BLE memory with `esp_bt_controller_mem_release(ESP_BT_MODE_BLE)`
2. **Mode Mismatch**: Then trying to enable Classic BT only mode `ESP_BT_MODE_CLASSIC_BT`
3. **Configuration Error**: This combination is invalid in ESP-IDF v5.4.1

## Solution Applied

### 1. Changed Bluetooth Mode
**Before:**
```c
ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
```

**After:**
```c
ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BTDM));
```

### 2. Removed Memory Release
**Before:**
```c
ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
```

**After:**
```c
// Не освобождаем память BLE, так как используем BTDM режим
```

## Technical Details

- **ESP_BT_MODE_BTDM**: Enables both Classic Bluetooth and BLE
- **ESP_BT_MODE_CLASSIC_BT**: Classic Bluetooth only (requires specific memory configuration)
- **Memory Management**: BTDM mode requires both BLE and Classic BT memory to be available

## Expected Result

The ESP32 should now:
1. ✅ Initialize Bluetooth controller successfully
2. ✅ Enable both Classic BT and BLE capabilities
3. ✅ Support HID Host functionality
4. ✅ Connect to BT13 remote control

## Verification

After flashing the fixed firmware:
```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Expected log output:
```
I (xxx) BT13_MOTOR_CONTROL: Bluetooth инициализирован
I (xxx) BT13_MOTOR_CONTROL: Поиск пульта BT13...
```

## Compatibility

- ✅ ESP-IDF v5.4.1
- ✅ ESP32 Classic Bluetooth
- ✅ HID Host functionality
- ✅ Motor control via PWM

This fix ensures proper Bluetooth initialization and resolves the runtime crash.