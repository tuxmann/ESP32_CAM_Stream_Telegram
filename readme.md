# ESP32-CAM Setup Guide

## Arduino IDE + Local Streaming + Telegram Bot

---

## Part 1: Hardware Setup

### What You'll Need

- ESP32-CAM board
- FTDI programmer (or USB-to-TTL adapter)
- Jumper wires
- Micro USB cable
- 5V power supply (for stable operation)

### Wiring for Programming

Connect your FTDI programmer to ESP32-CAM:

```
FTDI          ESP32-CAM
----          ---------
GND     ->    GND
5V      ->    5V
TX      ->    U0R (RX)
RX      ->    U0T (TX)
```

**Important**: Connect GPIO 0 to GND to enable programming mode. Disconnect after uploading.

---

## Part 2: Arduino IDE Setup

### Step 1: Install ESP32 Board Support

1. Open Arduino IDE

2. Go to **File → Preferences**

3. In "Additional Board Manager URLs", add:
   
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

4. Go to **Tools → Board → Boards Manager**

5. Search for "esp32" and install "ESP32 by Espressif Systems"

### Step 2: Select Board

1. Go to **Tools → Board → ESP32 Arduino**
2. Select **AI Thinker ESP32-CAM**
3. Set the following:
   - Upload Speed: 115200
   - Flash Frequency: 80MHz
   - Partition Scheme: Huge APP (3MB No OTA)

### Step 3: Install Required Libraries

Go to **Sketch → Include Library → Manage Libraries** and install:

- **UniversalTelegramBot** by Brian Lough
- **ArduinoJson** (version 6.x)

---

## Part 3: Create Telegram Bot

### Get Your Bot Token

1. Open Telegram and search for **@BotFather**
2. Send `/newbot` command
3. Follow prompts to name your bot
4. Copy the **Bot Token** (looks like: 123456789:ABCdefGHIjklMNOpqrsTUVwxyz)

### Get Your Chat ID

1. Search for **@userinfobot** on Telegram
2. Start a chat - it will reply with your Chat ID
3. Copy this number (e.g., 987654321)

---

## Part 4: Upload the Code

### Step 1: Open the Sketch

1. Open `ESP32_CAM_Stream_Telegram.ino` in Arduino IDE
2. Replace the following values at the top of the code:
   - `YOUR_WIFI_SSID` → Your WiFi network name
   - `YOUR_WIFI_PASSWORD` → Your WiFi password
   - `YOUR_BOT_TOKEN` → Your Telegram bot token
   - `YOUR_CHAT_ID` → Your Telegram chat ID

### Step 2: Upload

1. **Connect GPIO 0 to GND** (this enables programming mode)
2. Connect ESP32-CAM to your FTDI programmer
3. Plug USB into computer
4. Select the correct COM port in **Tools → Port**
5. Click **Upload** button
6. Wait for "Connecting..." to appear
7. **Press the RESET button on ESP32-CAM** when you see "Connecting..."
8. Wait for upload to complete (takes 1-2 minutes)

### Step 3: Run the Program

1. **Disconnect GPIO 0 from GND**
2. Press RESET button on ESP32-CAM
3. Open Serial Monitor (115200 baud)
4. Watch for WiFi connection and IP address

---

## Part 5: Using Your ESP32-CAM

### Local Network Streaming

1. Find the IP address in Serial Monitor (e.g., 192.168.1.100)
2. Open browser on any device on your network
3. Go to: `http://[YOUR_IP_ADDRESS]`
4. You'll see live video stream!

### Telegram Commands

Send these commands to your bot:

- `/start` - Show available commands
- `/photo` - Take and receive a photo
- `/flash` - Toggle the flash LED on/off
- `/status` - Get WiFi signal and IP address

---

## Troubleshooting

### "Camera init failed"

- Check camera ribbon cable is properly inserted
- Make sure you selected "AI Thinker ESP32-CAM" board
- Try using an external 5V power supply

### "Failed to connect to WiFi"

- Double-check SSID and password (case-sensitive!)
- Make sure you're using 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Move closer to router

### "Brownout detector was triggered"

- Power supply is insufficient
- Use external 5V power supply with at least 1A
- Don't power from FTDI programmer while using camera

### Can't upload / "Timed out waiting for packet header"

- Make sure GPIO 0 is connected to GND before uploading
- Press RESET button when "Connecting..." appears
- Check all wiring connections
- Try lowering upload speed to 57600

### Stream is choppy

- Reduce `frame_size` in code (try FRAMESIZE_SVGA or FRAMESIZE_VGA)
- Check WiFi signal strength
- Reduce number of devices on your network

### Telegram photos not sending

- Verify bot token and chat ID are correct
- Check internet connection
- Make sure date/time on ESP32 is correct (handled automatically by WiFi)

### Getting "Unauthorized user" message

- Make sure you're using YOUR chat ID, not someone else's
- Send a message to @userinfobot to verify your Chat ID

---

## Tips for Best Results

### Image Quality

- Use external power supply for best stability
- Adjust `jpeg_quality` (lower = better, range 0-63)
- Change `frame_size` for different resolutions:
  - FRAMESIZE_QVGA (320x240) - Fast, low quality
  - FRAMESIZE_VGA (640x480) - Good balance
  - FRAMESIZE_SVGA (800x600) - Higher quality
  - FRAMESIZE_UXGA (1600x1200) - Best quality (requires PSRAM)

### Power Considerations

- Camera uses a lot of power when active
- Flash LED uses additional power
- Always use 5V power supply with 1A+ capacity
- Consider adding a capacitor (100-470µF) across power lines

### Security

- Change default bot token regularly
- Only share your Chat ID with trusted people
- Consider adding password protection to web stream
- Keep your ESP32 firmware updated

---

## Next Steps

### Enhancements You Can Add

1. **Motion Detection** - Trigger photos automatically
2. **PIR Sensor** - Detect movement with external sensor
3. **Time-lapse** - Take photos at intervals
4. **Multiple Users** - Allow multiple Telegram users
5. **SD Card Storage** - Save photos locally
6. **Battery Power** - Use deep sleep for battery operation

### Resources

- ESP32-CAM Documentation: https://github.com/espressif/esp32-camera
- Arduino ESP32 Reference: https://docs.espressif.com/projects/arduino-esp32/
- Telegram Bot API: https://core.telegram.org/bots/api
- ESP32 CAM Case[ESP32 CAM CASE by Anin](https://www.printables.com/model/280978-esp32-cam-case) 
  - licensed under a Creative Commons (4.0 International License)
    Attribution-NonCommercial

---

## Need Help?

If you run into issues:

1. Check Serial Monitor output for error messages
2. Verify all wiring connections
3. Test each feature separately (streaming, then Telegram)
4. Check your WiFi and internet connection
5. Review the Troubleshooting section above

Happy streaming! 📸
