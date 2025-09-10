# ESP32 Over-The-Air (OTA) Upload Guide

## 🚀 Quick Start

### **Step 1: First Upload (Serial)**
Upload the OTA-enabled code via USB cable first:
```bash
# Make sure ESP32 is connected via USB
pio run --target upload --environment esp32dev
```

### **Step 2: Find ESP32 IP Address**
After the first upload, check the serial monitor for the IP address:
```
WiFi connected
IP address: 192.168.1.xxx
🔄 OTA Ready! You can now upload wirelessly.
   Hostname: esp32-light-controller
   IP: 192.168.1.xxx
   Port: 3232
   Password: lightota2024
```

### **Step 3: Wireless Upload**
```bash
# Upload via OTA (wireless)
pio run --target upload --environment esp32dev_ota
```

## 📋 Prerequisites

### **Network Requirements:**
- ✅ ESP32 and computer on same WiFi network
- ✅ Network allows device discovery (no AP isolation)
- ✅ Firewall allows connections on port 3232

### **Software Requirements:**
- ✅ PlatformIO installed
- ✅ ESP32 running OTA-enabled firmware
- ✅ Python (for espota tool)

## 🔧 Configuration Options

### **Method 1: Using Hostname (Default)**
```ini
upload_port = esp32-light-controller.local
```

### **Method 2: Using IP Address**
If hostname doesn't work, use IP address:
```ini
upload_port = 192.168.1.100  ; Replace with your ESP32's actual IP
```

### **Custom OTA Settings**
Edit these in `main.cpp` if needed:
```cpp
ArduinoOTA.setHostname("esp32-light-controller");  // Device name
ArduinoOTA.setPassword("lightota2024");            // OTA password
ArduinoOTA.setPort(3232);                          // OTA port
```

## 🔍 Troubleshooting

### **Problem: "Host not found"**
```bash
Error: Host esp32-light-controller.local not found
```
**Solutions:**
1. Use IP address instead of hostname
2. Check mDNS is working: `ping esp32-light-controller.local`
3. Update platformio.ini:
   ```ini
   upload_port = 192.168.1.100  ; Your ESP32's IP
   ```

### **Problem: "Authentication Failed"**
```bash
Error: Authentication Failed
```
**Solutions:**
1. Check password matches in code and platformio.ini
2. Verify the ESP32 is running OTA-enabled firmware
3. Restart ESP32 and try again

### **Problem: "Connection Timeout"**
```bash
Error: Connection timeout
```
**Solutions:**
1. Ensure ESP32 and computer are on same network
2. Check firewall settings (allow port 3232)
3. Verify ESP32 is powered and running
4. Check network allows device-to-device communication

### **Problem: "Upload Failed During Transfer"**
**Solutions:**
1. ESP32 might be busy - wait and retry
2. Restart ESP32 if voice/MQTT operations are interfering
3. Try upload when ESP32 is idle

## 📱 Finding ESP32 IP Address

### **Method 1: Serial Monitor**
Connect via USB and check serial output:
```bash
pio device monitor --environment esp32dev
```

### **Method 2: Router Admin Panel**
Check your router's connected devices list for "esp32-light-controller"

### **Method 3: Network Scanner**
```bash
# Linux/Mac
nmap -sn 192.168.1.0/24 | grep -A2 "esp32"

# Or use arp
arp -a | grep esp32
```

### **Method 4: MQTT Heartbeat**
Check MQTT topic `devices/esp32-light-controller/heartbeat` for IP address

## 🎯 Upload Commands

### **Standard Serial Upload:**
```bash
pio run --target upload --environment esp32dev
```

### **OTA Upload:**
```bash
pio run --target upload --environment esp32dev_ota
```

### **Upload with Monitoring:**
```bash
# OTA upload and immediately start monitoring
pio run --target upload --environment esp32dev_ota && pio device monitor
```

### **Force Upload (if stuck):**
```bash
# Add verbose output for debugging
pio run --target upload --environment esp32dev_ota --verbose
```

## ⚙️ Advanced Configuration

### **Custom Upload Script**
Create `upload_ota.sh`:
```bash
#!/bin/bash
ESP32_IP="192.168.1.100"  # Replace with your IP
echo "Uploading to ESP32 at $ESP32_IP..."
pio run --target upload --environment esp32dev_ota
echo "Upload completed!"
```

### **Auto-Discovery Script**
Create `find_esp32.py`:
```python
import socket
import subprocess

def find_esp32():
    try:
        ip = socket.gethostbyname('esp32-light-controller.local')
        print(f"ESP32 found at: {ip}")
        return ip
    except:
        print("ESP32 not found via hostname")
        return None

if __name__ == "__main__":
    find_esp32()
```

## 🔒 Security Notes

### **OTA Password**
- Current password: `lightota2024`
- Change in both `main.cpp` and `platformio.ini`
- Use strong passwords for production

### **Network Security**
- OTA is only available when ESP32 is connected to WiFi
- Consider disabling OTA in production builds
- Monitor OTA attempts via serial/MQTT

## 🎵 Audio Feedback During OTA

The ESP32 provides audio feedback during OTA updates:
- **Update Start:** Two ascending tones (1000Hz → 1200Hz)
- **Update Success:** Three ascending tones (800Hz → 1000Hz → 1200Hz)
- **Update Error:** Standard error sound (400Hz → 300Hz)

Voice detection is automatically disabled during OTA updates for safety.

## 📊 Monitoring OTA Updates

### **Serial Output During OTA:**
```
🔄 OTA Update starting (sketch)
📊 OTA Progress: 25%
📊 OTA Progress: 50%
📊 OTA Progress: 75%
📊 OTA Progress: 100%
✅ OTA Update completed successfully
```

### **MQTT Integration:**
- Voice detection status published in heartbeat
- OTA progress could be added to MQTT topics if needed

## 🚨 Emergency Recovery

If OTA updates fail and ESP32 becomes unresponsive:

1. **Power Cycle:** Unplug and reconnect power
2. **Serial Recovery:** Connect USB cable and upload via serial
3. **Factory Reset:** Hold BOOT button while powering on (if needed)

The ESP32 will always accept serial uploads regardless of OTA state.
