# ESP32 DS18B20 Realtime Temperature Monitor

A real-time temperature monitoring system using ESP32 and DS18B20 sensors with a responsive web dashboard and WebSocket communication.

## 🌡️ Features

- **Real-time monitoring** of multiple DS18B20 temperature sensors
- **WebSocket-based** live updates (~1Hz refresh rate)
- **Responsive web dashboard** accessible from any device
- **Multiple sensor support** on a single OneWire bus
- **Automatic sensor discovery** with unique addressing
- **Temperature color coding** (green/yellow/red based on values)
- **Connection status monitoring** with auto-reconnect
- **Minimal, modern UI** optimized for mobile and desktop

## 📋 Requirements

### Hardware
- ESP32 development board
- One or more DS18B20 temperature sensors
- 4.7kΩ pull-up resistor
- Breadboard and jumper wires

### Software Dependencies
- [Arduino IDE](https://www.arduino.cc/en/software) or [PlatformIO](https://platformio.org/)
- ESP32 Arduino Core
- Required Libraries:
  - `OneWire`
  - `DallasTemperature` 
  - `ESPAsyncWebServer` [ESP32Async](https://github.com/ESP32Async/ESPAsyncWebServer)
  - `AsyncTCP` [AsyncTCP](https://github.com/ESP32Async/AsyncTCP)

## 🔌 Hardware Setup

### Wiring Diagram
```
ESP32           DS18B20 Sensor(s)
-----           -----------------
3.3V    ------> VDD (Red)
GND     ------> GND (Black)
GPIO15  ------> DQ  (Yellow/Data)

Pull-up Resistor: 4.7kΩ between VDD and DQ
```

### Multiple Sensors
You can connect multiple DS18B20 sensors in parallel on the same OneWire bus:
- All VDD pins connect to 3.3V
- All GND pins connect to GND  
- All DQ pins connect to GPIO15
- Only one 4.7kΩ pull-up resistor needed

## ⚙️ Software Installation

### 1. Install Required Libraries

**Using Arduino IDE:**
1. Go to `Tools` → `Manage Libraries`
2. Search and install:
   - "OneWire"
   - "DallasTemperature" 
   - "ESPAsyncWebServer"
   - "AsyncTCP"


### 2. Configure WiFi Credentials

Edit the configuration section in `esp_temp_read_and_show_html.ino`:

```cpp
// ===================== User config =====================
const char *WIFI_SSID = "your_wifi_network";
const char *WIFI_PASSWORD = "your_wifi_password";

#define ONE_WIRE_BUS 15        // GPIO for DS18B20 data
#define READ_INTERVAL_MS 1000  // How often to read and broadcast
// =======================================================
```

### 3. Upload to ESP32

1. Connect your ESP32 to your computer
2. Select the correct board and port in Arduino IDE
3. Upload the sketch

## 🚀 Usage

1. **Power on** your ESP32 with connected DS18B20 sensors
2. **Check Serial Monitor** (115200 baud) for:
   - WiFi connection status
   - ESP32 IP address
   - Detected sensor addresses
3. **Open web browser** and navigate to the ESP32's IP address
4. **Monitor temperatures** in real-time on the dashboard

### Web Dashboard Features

- **Live temperature readings** with automatic updates
- **Color-coded values**:
  - 🟢 Green: Normal (0-39°C)
  - 🟡 Yellow: Warning (40-60°C)  
  - 🔴 Red: Critical (<0°C or >60°C)
- **Sensor identification** by unique address
- **Connection status** indicator
- **Update interval** display
- **Mobile-responsive** design

## 📊 Data Format

The system broadcasts JSON data via WebSocket:

```json
{
  "type": "temps",
  "ts": 12345678,
  "sensors": [
    {
      "addr": "28ABC123456789DE", 
      "c": 23.75
    },
    {
      "addr": "28DEF987654321AB",
      "c": 24.12
    }
  ]
}
```

## 🔧 Configuration Options

### Temperature Reading Interval
Modify `READ_INTERVAL_MS` to change update frequency:
```cpp
#define READ_INTERVAL_MS 1000  // 1 second (1000ms)
```

### OneWire GPIO Pin
Change the data pin for DS18B20 sensors:
```cpp
#define ONE_WIRE_BUS 15  // Use GPIO 15
```

### Sensor Resolution
Adjust temperature precision in `setup()`:
```cpp
sensors.setResolution(9);   // 9-bit: ±0.5°C, ~100ms
sensors.setResolution(10);  // 10-bit: ±0.25°C, ~200ms  
sensors.setResolution(11);  // 11-bit: ±0.125°C, ~400ms
sensors.setResolution(12);  // 12-bit: ±0.0625°C, ~750ms
```

## 🌐 API Endpoints

- `GET /` - Web dashboard interface
- `WebSocket /ws` - Real-time temperature data stream

## 🐛 Troubleshooting

### No Sensors Detected
- Check wiring connections
- Verify 4.7kΩ pull-up resistor is connected
- Ensure DS18B20 sensors are genuine (many clones exist)
- Try different GPIO pin

### WiFi Connection Issues  
- Verify SSID and password are correct
- Check WiFi signal strength
- Ensure ESP32 is within range of router

### Web Dashboard Not Loading
- Check Serial Monitor for ESP32 IP address
- Ensure ESP32 and client device are on same network
- Try accessing `http://[ESP32_IP]/` directly

### Temperature Readings Show Error
- Check sensor wiring
- Verify power supply (3.3V)
- Replace potentially faulty sensors
- Check for loose connections

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📸 Screenshots

The web dashboard provides a clean, modern interface showing:
- Real-time temperature readings
- Sensor identification by unique address
- Connection status and update intervals
- Color-coded temperature warnings
- Responsive design for all devices

---

**Enjoy monitoring your temperatures in real-time! 🌡️**
