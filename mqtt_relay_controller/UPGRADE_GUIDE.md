# ESP32 Audio Upgrade Guide

## Overview
This guide explains how to upgrade your existing ESP32 MQTT light controller to support voice commands and audio feedback.

## Files Changed

### 1. New Files Created
- `main_with_audio.cpp` - Enhanced firmware with audio support
- `AUDIO_WIRING_DIAGRAM.md` - Complete wiring diagram
- `UPGRADE_GUIDE.md` - This guide

### 2. Modified Files
- `platformio.ini` - Updated with ESP32 core library

## Upgrade Steps

### Step 1: Backup Current Setup
```bash
# Backup your current main.cpp
cp src/main.cpp src/main_backup.cpp
```

### Step 2: Replace Firmware
```bash
# Replace main.cpp with the audio-enabled version
cp src/main_with_audio.cpp src/main.cpp
```

### Step 3: Build and Upload
```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

## Hardware Modifications Required

### New Components Needed
1. **INMP441 Microphone Module** - Rs. 840.00
2. **PAM8610 Amplifier Board** - Rs. 280.00
3. **8Ω 10W Speaker** - Rs. 290.00
4. **12V 3A Power Supply** (recommended)
5. **Resistors**: 2x 1kΩ for audio protection
6. **Connecting wires**

### Wiring Changes
Follow the complete wiring diagram in `AUDIO_WIRING_DIAGRAM.md`

**Key Connections:**
- GPIO 22, 25, 26 → INMP441 microphone
- GPIO 21 → PAM8610 amplifier input
- 12V supply → PAM8610 power
- Speaker → PAM8610 output

### Power Supply Upgrade
- **Current**: 5V supply for ESP32 + relay
- **New**: 12V 3A supply for full system
  - ESP32: 12V → 5V regulator → 3.3V onboard
  - Amplifier: 12V direct
  - Speaker: Via amplifier

## New Features Added

### 1. Voice Command Support
- **Commands**: "turn on", "turn off", "light on", "light off", "status"
- **Detection**: Continuous voice activity monitoring
- **Processing**: Basic pattern matching (extendable to cloud services)

### 2. Audio Feedback
- **Confirmation Sound**: Success beep (800Hz + 1000Hz)
- **Error Sound**: Error tone (400Hz + 300Hz)
- **Startup Sound**: System ready indication

### 3. Enhanced MQTT Features
- **New Topics**: 
  - `devices/esp32-light-controller/audio` - Voice command events
- **Enhanced Capabilities**: Voice detection enable/disable
- **Extended Heartbeat**: Includes audio system status

### 4. Advanced Configuration
- **Voice Detection**: Can be enabled/disabled via MQTT
- **Audio Thresholds**: Configurable voice activity detection
- **Multiple Interfaces**: MQTT + Voice commands work simultaneously

## Testing Procedure

### 1. Hardware Test
1. Power on system with 12V supply
2. Check serial output for initialization messages
3. Verify relay operation (existing functionality)
4. Test audio output with startup sound

### 2. Voice Test
1. Speak "turn on" near microphone
2. Listen for confirmation sound
3. Verify light turns on
4. Test "turn off" command
5. Check MQTT messages in audio topic

### 3. MQTT Integration Test
1. Send MQTT commands (existing functionality should work)
2. Test new voice enable/disable commands
3. Verify heartbeat includes audio status

## Configuration Options

### Voice Detection Settings
```cpp
const int DETECTION_THRESHOLD = 1000;  // Adjust sensitivity
const unsigned long voiceTimeoutMs = 3000; // Command timeout
bool voiceDetectionEnabled = true; // Enable/disable voice
```

### Audio Output Settings
```cpp
const int AUDIO_OUT_PIN = 21; // Output pin to amplifier
// Tone frequencies for feedback
playTone(800, 100);  // Confirmation
playTone(400, 200);  // Error
```

## Troubleshooting

### No Audio Output
- Check 12V power supply connection
- Verify PAM8610 wiring
- Check speaker polarity
- Monitor amplifier temperature

### Voice Not Detected
- Increase `DETECTION_THRESHOLD` if too sensitive
- Decrease if not sensitive enough
- Check I2S microphone connections
- Verify 3.3V power to INMP441

### MQTT Issues
- All existing MQTT functionality preserved
- New audio topic may need broker permissions
- Check enhanced heartbeat format

### Power Issues
- 12V supply must handle 3A current
- Check voltage regulation to 3.3V
- Monitor current consumption

## Future Enhancements

### Cloud Speech Recognition
Replace the basic pattern matching with:
- Google Speech-to-Text API
- Amazon Transcribe
- Microsoft Speech Services

### Advanced Audio
- Text-to-Speech responses
- Custom audio messages
- Multi-language support

### Smart Home Integration
- Home Assistant integration
- Alexa/Google Assistant compatibility
- Custom wake word detection

## Rollback Procedure

If you need to revert to the original firmware:

```bash
# Restore original firmware
cp src/main_backup.cpp src/main.cpp

# Build and upload
pio run --target upload
```

Remove audio components and revert to 5V power supply.

## Support

For issues or questions:
1. Check serial monitor output for error messages
2. Verify all wiring connections
3. Test individual components separately
4. Review configuration parameters


