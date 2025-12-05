# Christmas Countdown Timer

An ESP32-based countdown timer that displays the number of days until Christmas on a 4-digit 7-segment display. The device connects to WiFi, syncs time via NTP, and updates the countdown automatically.

## Hardware Requirements

- **Microcontroller**: Adafruit Feather ESP32-S3 (no PSRAM)
- **Display**: Adafruit HT16K33 4-digit 7-segment display
- **Connection**: I2C (default address 0x70)

## Features

- **WiFi with Fallback**: Configure multiple WiFi networks with automatic fallback
- **NTP Time Sync**: Automatically syncs time from NTP servers
- **Configurable Hostname**: Set custom network hostname for easy identification
- **Timezone Support**: Configurable GMT offset and daylight saving time
- **Auto-updating**: Recalculates countdown every 60 seconds
- **Visual Feedback**: Display animations during WiFi connection
- **Serial Debugging**: Detailed status output via serial console

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

1. Initializes the 7-segment display
2. Shows "8888" test pattern
3. Sets network hostname
4. Attempts to connect to WiFi networks (tries each for 10 seconds)
5. Syncs time from NTP server
6. Begins countdown display

### Main Loop

Every 60 seconds:
1. Gets current time from the ESP32's RTC
2. Calculates days until December 25
3. Updates the display
4. Prints status to serial console

### Special Behaviors

- **Christmas Day**: Displays "0" with blinking colon
- **After Christmas**: Counts down to next year's Christmas
- **WiFi Failure**: Displays "-1" and halts (all configured networks failed)
- **Connection Progress**: Animates digits while connecting to WiFi

## Display Output

The 4-digit display shows:
- `8888`: Test pattern on startup
- `0-9`: Animated counter during WiFi connection
- `21`: Days until Christmas (example)
- `0` (blinking): It's Christmas!
- `-1`: Error - no WiFi connection

## Serial Console Output

Example output:
```
Christmas Countdown Timer
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
Today: 12/4/2025 - Days until Christmas 2025: 21
```

## Troubleshooting

### Display Not Found
- Check I2C connections (SDA/SCL)
- Verify display address (default is 0x70)
- Test with I2C scanner sketch

### WiFi Won't Connect
- Verify network credentials in config.h
- Check signal strength
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Try increasing `maxAttempts` in main.cpp

### Time Sync Fails
- Verify internet connectivity
- Check firewall isn't blocking NTP (UDP port 123)
- Try different NTP server (time.google.com, time.nist.gov)

### Wrong Countdown
- Verify timezone settings (`gmtOffset_sec`)
- Check daylight saving time setting (`daylightOffset_sec`)
- Monitor serial output to see what date/time is being used

## File Structure

```
countdown_clock/
├── platformio.ini          # PlatformIO configuration
├── src/
│   ├── main.cpp           # Main program code
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

## Credits

Built with:
- [PlatformIO](https://platformio.org/)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit LED Backpack Library](https://github.com/adafruit/Adafruit_LED_Backpack)
- ESP32 Arduino Core
