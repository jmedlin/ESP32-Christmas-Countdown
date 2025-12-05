# Christmas Countdown Timer

An ESP32-based countdown timer that displays the number of days until Christmas on a 4-digit 7-segment display. The device connects to WiFi, syncs time via NTP, and updates the countdown automatically.

## Hardware Requirements

- **Microcontroller**: Adafruit Feather ESP32-S3 (no PSRAM)
- **Display**: Adafruit HT16K33 4-digit 7-segment display
- **Battery Monitor**: MAX17048 LiPo Fuel Gauge (built-in on Feather ESP32-S3)
- **Power**: LiPo battery connected to onboard BMS
- **Connection**: I2C (display at 0x70, fuel gauge auto-detected)

## Features

### Core Features
- **WiFi with Fallback**: Configure multiple WiFi networks with automatic fallback
- **Optional WiFi Operation**: Device can start and run without WiFi connection
- **NTP Time Sync**: Automatically syncs time from NTP servers on startup
- **Periodic NTP Re-sync**: Automatically re-syncs time every hour when WiFi is available
- **WiFi Auto-reconnect**: Attempts to reconnect to WiFi every hour if connection is lost
- **Configurable Hostname**: Set custom network hostname for easy identification
- **Timezone Support**: Configurable GMT offset and daylight saving time
- **Auto-updating**: Recalculates countdown every 60 seconds
- **Visual Feedback**: Display animations during WiFi connection
- **Serial Debugging**: Detailed status output via serial console

### Web Interface
- **Live Status Page**: Beautiful web interface showing countdown and system info
- **Real-time Updates**: Status page auto-refreshes every 5 seconds
- **JSON API**: `/api/status` endpoint provides all system data in JSON format
- **Network Information**: Displays WiFi SSID, IP address, and signal strength

### Battery Management
- **Accurate Battery Monitoring**: Uses MAX17048 fuel gauge for precise voltage and percentage readings
- **Charging Status Detection**: Displays charging, charged, discharging, or on battery status
- **Battery Protection**: Automatically enters deep sleep when battery drops below 3.0V or 5%
- **Auto-wake**: Wakes every hour to check if battery has been recharged
- **Visual Battery Indicator**: Web interface shows battery status with color-coded progress bar
- **Serial Battery Reports**: Battery voltage, percentage, and status in console output

### Power Management
- **Deep Sleep Mode**: Ultra-low power consumption (~10µA) when battery is critically low
- **Battery-first Design**: Optimized for portable, battery-powered operation
- **Smart Wake-up**: Device resumes normal operation once battery is recharged

## Setup

### 1. Install PlatformIO

Install PlatformIO Core via pip:
```bash
pip install platformio
```

Or via Homebrew (macOS):
```bash
brew install platformio
```

### 2. Configure Your Settings

Copy the example config file:
```bash
cp src/config.h.example src/config.h
```

Edit `src/config.h` with your settings:

```cpp
// WiFi networks (tries in order until one connects)
const WiFiNetwork wifiNetworks[] = {
  {"Your_Home_Network", "your_password"},
  {"Your_Work_Network", "work_password"},
  {"Your_Phone_Hotspot", "hotspot_password"}
};

// Network hostname (how device appears on network)
const char* hostname = "xmas-countdown";

// NTP settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;  // EST is UTC-5 (-18000), PST is UTC-8 (-28800)
const int daylightOffset_sec = 3600;  // 3600 for DST, 0 for standard time

// Display settings
const uint8_t displayAddress = 0x70;  // I2C address
const uint8_t displayBrightness = 15;  // 0-15, where 15 is brightest
```

### 3. Prepare Your ESP32-S3

If your board was previously running CircuitPython, erase the flash:

```bash
# Find your port
ls /dev/cu.*

# Erase flash (replace PORT with your actual port)
esptool.py --chip esp32s3 --port /dev/cu.PORT erase_flash
```

For the first upload after erasing, you may need to:
1. Hold the BOOT button
2. Press RESET (or plug in USB)
3. Release BOOT
4. Run the upload command

### 4. Build and Upload

```bash
# Build the project
pio run

# Upload to device
pio run --target upload

# Upload and monitor serial output
pio run --target upload && pio device monitor
```

### 5. Monitor Serial Output

View debug output and status:
```bash
pio device monitor
```

Press `Ctrl+C` to exit the monitor.

## How It Works

### Startup Sequence

1. Initializes serial communication (115200 baud)
2. Checks if waking from deep sleep (battery check)
3. Initializes MAX17048 fuel gauge and reads battery status
4. If battery critically low, returns to deep sleep
5. Initializes the 7-segment display
6. Shows "8888" test pattern
7. Sets network hostname
8. Attempts to connect to WiFi networks (tries each for 10 seconds)
9. If WiFi available: Syncs time from NTP server
10. If no WiFi: Continues with internal RTC (time may be inaccurate)
11. Starts web server (if WiFi connected)
12. Begins countdown display

### Main Loop

Every 60 seconds:
1. Checks if 1 hour has passed since last NTP sync
2. If yes and WiFi available: Re-syncs time from NTP
3. If WiFi disconnected: Attempts to reconnect
4. Gets current time from the ESP32's RTC
5. Calculates days until December 25
6. Updates battery status from MAX17048
7. Checks for critically low battery (enters deep sleep if needed)
8. Updates the display
9. Prints status to serial console (date, time, countdown, battery info)
10. Every 5 minutes: Prints detailed network status

### Special Behaviors

- **Christmas Day**: Displays "0" with blinking colon
- **After Christmas**: Counts down to next year's Christmas
- **No WiFi at Startup**: Continues operation with internal RTC, attempts reconnection every hour
- **WiFi Lost During Operation**: Continues countdown, attempts reconnection every hour
- **Low Battery Warning**: Enters deep sleep to protect battery when below 3.0V or 5%
- **Deep Sleep Recovery**: Wakes every hour to check battery status
- **Battery Charged**: Automatically resumes normal operation when battery recovers
- **Connection Progress**: Animates digits while connecting to WiFi

## Display Output

The 4-digit display shows:
- `8888`: Test pattern on startup
- `0-9`: Animated counter during WiFi connection
- `21`: Days until Christmas (example)
- `0` (blinking): It's Christmas!
- `DEAD`: Low battery warning (shown for 2 seconds before deep sleep)

## Serial Console Output

Example output:
```
Christmas Countdown Timer
MAX17048 fuel gauge initialized
Battery Voltage: 3.87V
Battery Percent: 73.0%
Display initialized
Hostname set to: xmas-countdown
Trying network: Your_Home_Network
........
WiFi connected!
Connected to: Your_Home_Network
IP address: 192.168.1.123
Getting time from NTP server...
Time synchronized!
Wednesday, December 4 2025 14:30:25

Web server started!
Visit: http://192.168.1.123

Today: 12/4/2025 14:30:25 - Days until Christmas 2025: 21 - Battery: 3.87V (73%) - On Battery

--- Network Status ---
Connected to: Your_Home_Network
IP Address: 192.168.1.123
Signal Strength: -45 dBm
Last NTP sync: 300 seconds ago
---------------------
```

## Web Interface

Access the web interface by navigating to the device's IP address (shown in serial console):

```
http://192.168.1.123
```

The web page displays:
- Large countdown number showing days until Christmas
- Current date and time
- Network status (SSID, IP, signal strength)
- **Battery status** (voltage, percentage, charging state)
- **Color-coded battery bar**:
  - Green: Normal battery level
  - Blue: Charging or fully charged
  - Red: Low battery (< 20%)

The page automatically refreshes every 5 seconds to show real-time updates.

### API Endpoint

JSON data is available at `/api/status`:

```json
{
  "days": 21,
  "christmasYear": 2025,
  "year": 2025,
  "month": 12,
  "day": 4,
  "hour": 14,
  "min": 30,
  "sec": 25,
  "ssid": "Your_Network",
  "ip": "192.168.1.123",
  "rssi": -45,
  "batteryVoltage": 3.87,
  "batteryPercent": 73,
  "batteryStatus": "On Battery"
}
```

## Battery Configuration

Low battery thresholds can be adjusted in `main.cpp`:

```cpp
#define LOW_BATTERY_VOLTAGE 3.0  // Voltage threshold (default: 3.0V)
#define LOW_BATTERY_PERCENT 5    // Percentage threshold (default: 5%)
```

NTP sync interval can be adjusted:

```cpp
const unsigned long ntpSyncInterval = 3600000;  // Default: 1 hour (in milliseconds)
```

## Troubleshooting

### Display Not Found
- Check I2C connections (SDA/SCL)
- Verify display address (default is 0x70)
- Test with I2C scanner sketch

### Battery Monitor Not Found
- Ensure you're using an Adafruit Feather ESP32-S3 with built-in MAX17048
- Check I2C connections
- Device will continue to work without battery monitoring

### WiFi Won't Connect
- Device will continue to operate without WiFi using internal RTC
- Verify network credentials in config.h
- Check signal strength
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Device will attempt to reconnect every hour

### Time Sync Fails
- Device will continue to use internal RTC
- Verify internet connectivity
- Check firewall isn't blocking NTP (UDP port 123)
- Try different NTP server (time.google.com, time.nist.gov)
- Device will retry NTP sync every hour when WiFi is available

### Wrong Countdown
- Verify timezone settings (`gmtOffset_sec`)
- Check daylight saving time setting (`daylightOffset_sec`)
- Monitor serial output to see what date/time is being used
- Time will automatically re-sync every hour if WiFi is available

### Device Keeps Entering Deep Sleep
- Battery is below 3.0V or 5% - charge the battery
- Device will automatically wake and resume when battery is charged
- Adjust thresholds in main.cpp if needed for your battery type

### Web Interface Not Accessible
- Check that device successfully connected to WiFi (view serial output)
- Verify you're on the same network as the device
- Try accessing by IP address shown in serial console
- Web server only starts if WiFi connection succeeds

## File Structure

```
countdown_clock/
├── platformio.ini          # PlatformIO configuration
├── src/
│   ├── main.cpp           # Main program code
│   ├── webpage.h          # Web interface HTML/CSS/JavaScript
│   ├── config.h           # Your settings (gitignored)
│   └── config.h.example   # Template for config.h
├── .gitignore             # Git ignore rules
└── README.md              # This file
```

## Configuration Files

- **config.h**: Your actual configuration (gitignored, not committed)
- **config.h.example**: Template showing configuration format (committed to repo)

When sharing or cloning this project, copy `config.h.example` to `config.h` and update with your credentials.

## License

This project is provided as-is for personal and educational use.

## Use Cases

This countdown timer is perfect for:

- **Portable Christmas Countdown**: Battery-powered operation allows placement anywhere
- **Always-On Display**: Low power consumption suitable for 24/7 operation
- **Remote Monitoring**: Web interface accessible from any device on your network
- **Office Desk Gadget**: Automatic WiFi reconnection handles network changes
- **Gift Project**: Self-contained with battery backup and automatic power management

## Technical Details

### Power Consumption
- **Active (WiFi on)**: ~160mA
- **Active (WiFi off)**: ~80mA
- **Deep Sleep**: ~10-150µA

### Battery Life Estimates
With a typical 500mAh LiPo battery:
- WiFi always on: ~3 hours
- WiFi periodic reconnect: ~5-6 hours
- Deep sleep: Several weeks

### Memory Usage
- Program Storage: ~1.2MB
- Dynamic Memory: ~50KB

## Credits

Built with:
- [PlatformIO](https://platformio.org/)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit LED Backpack Library](https://github.com/adafruit/Adafruit_LED_Backpack)
- [Adafruit MAX1704X Library](https://github.com/adafruit/Adafruit_MAX1704X)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- ESP32 Arduino Core
