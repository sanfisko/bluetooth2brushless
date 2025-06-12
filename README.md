# 🏄 ESP32 Bluetooth Motor Control

> 🌍 **English version** | **[Русская версия →](README_ru.md)**

ESP32 + Bluetooth HID remote = brushless motor control

## 🚀 Quick Start

### 🛒 Required Components:
- **Bluetooth remote BT13**: [AliExpress](https://a.aliexpress.com/_EIvUKYS) *(tested and working)*
- **Motor controller**: [AliExpress](https://a.aliexpress.com/_EyfVQSQ)
- **Brushless motor**: [AliExpress](https://a.aliexpress.com/_Exu4xp0)
- **ESP32 board**: [AliExpress](https://a.aliexpress.com/_ExqenUe)

### ⚡ Automatic Installation

**Standard installation:**
```bash
# Clone repository
git clone https://github.com/sanfisko/esp32-bluetooth-motor-control.git
cd esp32-bluetooth-motor-control

# Run automatic installation
./install.sh
```

> 📡 **install.sh** - automatic Bluetooth device discovery and MAC address configuration.

**The script automatically:**
- ✅ Checks system dependencies (git, python3, curl, pip, cmake)
- ✅ Installs ESP-IDF if not found
- ✅ Checks and offers ESP-IDF updates
- ✅ Activates ESP-IDF environment
- ✅ Builds the project
- ✅ Finds ESP32
- ✅ Flashes ESP32 (speed 115200 for reliability)
- ✅ Starts monitoring (exit: **Ctrl+]**)

## 📡 Compatible Remotes

**Recommended**: [BT13 remote from AliExpress](https://a.aliexpress.com/_EIvUKYS) - tested and working ✅

<details>
<summary><small>📋 Technical requirements for remote (for developers)</small></summary>

**Protocol**: Bluetooth HID (Human Interface Device)
**Connection**: Classic Bluetooth (not BLE)
**Buttons**: At least 3 buttons (forward, backward, stop)
**Range**: 10+ meters
**Battery**: Rechargeable preferred

**Tested models:**
- ✅ **BT13** - Full compatibility, all functions work
- ⚠️ **Other HID remotes** - May require code adaptation

**Button mapping (BT13):**
- **▲ (Up)** → Motor forward
- **▼ (Down)** → Motor backward  
- **⏸ (Middle)** → Motor stop
- **◀ ▶ (Left/Right)** → Reserved for future features

</details>

## 🔧 Manual Installation

<details>
<summary>Click to expand manual installation steps</summary>

### 1. Install ESP-IDF
```bash
# Install dependencies
sudo apt update
sudo apt install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Clone ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.4.1
git submodule update --init --recursive

# Install tools
./install.sh esp32

# Activate environment
source ~/esp/esp-idf/export.sh
```

### 2. Configure Bluetooth
```bash
# Find your BT13 MAC address
sudo hcitool scan
# or
bluetoothctl
> scan on
> devices
```

### 3. Update MAC in code
Edit `main/main.c` and replace MAC address:
```c
uint8_t target_mac[6] = {0x8B, 0xEB, 0x75, 0x4E, 0x65, 0x97}; // Your BT13 MAC
```

### 4. Build and flash
```bash
# Build
idf.py set-target esp32
idf.py build

# Flash (replace /dev/ttyUSB0 with your port)
idf.py -p /dev/ttyUSB0 flash monitor
```

</details>

## 🎮 Usage

1. **Turn on BT13**: Long press middle button until red+blue blinking
2. **Flash ESP32**: Run installation script
3. **Power on motor**: Connect battery to motor controller
4. **Control**:
   - **▲** - Forward
   - **▼** - Backward
   - **⏸** - Stop

## 📊 Connection Diagram

```
BT13 Remote  )))  ESP32  ←→  Motor Controller  ←→  Brushless Motor
    ↑                ↑              ↑                    ↑
Bluetooth HID    GPIO pins    PWM Signal           3-phase power
```

**ESP32 Connections:**
- **GPIO 18** → Motor controller PWM input
- **GPIO 19** → Motor controller direction
- **GPIO 21** → Status LED
- **3.3V/GND** → Motor controller logic power

## 🔍 Troubleshooting

<details>
<summary><strong>🚫 BT13 not connecting</strong></summary>

**Check:**
1. BT13 is in pairing mode (red+blue blinking)
2. BT13 is not connected to phone/computer
3. Correct MAC address in code
4. ESP32 Bluetooth is enabled

**Solutions:**
```bash
# Restart Bluetooth service
sudo systemctl restart bluetooth

# Clear Bluetooth cache
sudo rm -rf /var/lib/bluetooth/*
sudo systemctl restart bluetooth

# Check ESP32 logs
idf.py monitor
```

</details>

<details>
<summary><strong>⚡ Motor not responding</strong></summary>

**Check:**
1. Motor controller power supply
2. PWM signal connections (GPIO 18, 19)
3. Motor controller calibration
4. Battery voltage (minimum 11.1V for 3S)

**Debug:**
```bash
# Monitor ESP32 output
idf.py monitor

# Check PWM signals with multimeter
# GPIO 18 should show 1.65V (50% duty cycle) at rest
```

</details>

<details>
<summary><strong>🔧 Compilation errors</strong></summary>

**Common issues:**
```bash
# ESP-IDF not activated
source ~/esp/esp-idf/export.sh

# Wrong ESP-IDF version
cd ~/esp/esp-idf
git checkout v5.4.1

# Missing dependencies
sudo apt install cmake ninja-build

# Clean build
idf.py fullclean
idf.py build
```

</details>

## 🛠 Advanced Configuration

<details>
<summary>Motor Settings</summary>

Edit `main/main.c` to adjust motor parameters:

```c
// PWM frequency (Hz)
#define PWM_FREQUENCY 1000

// Speed limits (0-100%)
#define MAX_SPEED_FORWARD 80
#define MAX_SPEED_BACKWARD 60

// Acceleration (speed change per 100ms)
#define ACCELERATION_RATE 5

// Deadband (prevent accidental activation)
#define DEADBAND_THRESHOLD 10
```

</details>

<details>
<summary>Bluetooth Settings</summary>

```c
// Connection timeout (seconds)
#define BT_CONNECTION_TIMEOUT 30

// Reconnection attempts
#define MAX_RECONNECT_ATTEMPTS 5

// Signal strength threshold
#define MIN_RSSI_THRESHOLD -70
```

</details>

## 📈 Performance

- **Response time**: < 50ms
- **Range**: 10-15 meters (open space)
- **Battery life**: 
  - BT13: ~20 hours continuous use
  - ESP32: ~8 hours (with motor controller)
- **Motor control**: Smooth acceleration/deceleration

## 🔒 Safety Features

- **Automatic stop** on connection loss
- **Speed limiting** to prevent damage
- **Deadband** to prevent accidental activation
- **Watchdog timer** for system stability
- **Low battery detection** (if supported by controller)

## 🤝 Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Espressif** for ESP-IDF framework
- **Community** for testing and feedback
- **AliExpress sellers** for affordable components

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/sanfisko/esp32-bluetooth-motor-control/issues)
- **Discussions**: [GitHub Discussions](https://github.com/sanfisko/esp32-bluetooth-motor-control/discussions)
- **Email**: sanfisko@example.com

---

⭐ **Star this repository if it helped you!**