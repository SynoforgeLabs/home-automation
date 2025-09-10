# ESP32 Pin Assignment Validation

## Current Pin Usage Summary

| GPIO | Function | Component | Type | Notes |
|------|----------|-----------|------|-------|
| GPIO 4 | Relay Control | Light Relay | Digital Out | **EXISTING** - HIGH = ON |
| GPIO 21 | Audio Output | PAM8610 Input | Analog Out | **NEW** - To amplifier |
| GPIO 22 | I2S Clock | INMP441 SCK | Digital | **NEW** - Serial Clock |
| GPIO 25 | I2S Word Select | INMP441 WS | Digital | **NEW** - L/R Channel |
| GPIO 26 | I2S Data | INMP441 SD | Digital In | **NEW** - Audio data |

## Pin Conflict Analysis

### ✅ No Conflicts Detected
- All assigned pins are available on standard ESP32 dev boards
- No GPIO overlap between existing and new functionality
- I2S pins are grouped logically for clean wiring

### ESP32 Pin Capabilities Verification

#### GPIO 4 (Relay Control)
- ✅ Digital I/O capable
- ✅ No ADC/DAC/touch conflicts
- ✅ Safe for relay control (existing functionality preserved)

#### GPIO 21 (Audio Output)
- ✅ Digital I/O capable
- ✅ No built-in ADC/DAC on this pin
- ✅ Suitable for PWM audio output to amplifier
- ✅ No I2C/SPI conflicts in this configuration

#### GPIO 22 (I2S SCK)
- ✅ Commonly used for I2S serial clock
- ✅ No conflicts with system pins
- ✅ Good signal integrity for clock signals

#### GPIO 25 (I2S WS)
- ✅ Digital I/O capable
- ⚠️  Has DAC capability (DAC_CHANNEL_1) - not used in this config
- ✅ Perfect for I2S word select
- ✅ No conflicts with current usage

#### GPIO 26 (I2S SD)
- ✅ Digital input capable
- ⚠️  Has DAC capability (DAC_CHANNEL_2) - not used in this config
- ✅ Excellent for I2S serial data input
- ✅ No conflicts

## Reserved/System Pins (Not Used)

### Boot/Flash Pins (Avoided)
- GPIO 0 - Boot mode selection
- GPIO 2 - Boot mode, onboard LED
- GPIO 12 - Flash voltage selection
- GPIO 15 - Boot mode selection

### Power/Ground Pins
- 3.3V, 5V, GND - Used for component power
- EN - Reset pin (not modified)

### Other Available Pins (For Future Expansion)
- GPIO 5, 16, 17, 18, 19, 23, 27, 32, 33 - Available for sensors, etc.

## Power Distribution Validation

### Current Requirements
| Component | Voltage | Current | Power |
|-----------|---------|---------|-------|
| ESP32 | 3.3V | 250mA | 0.8W |
| Relay Module | 5V | 50mA | 0.25W |
| INMP441 Mic | 3.3V | 1.4mA | 0.005W |
| PAM8610 (10W) | 12V | 2A | 24W |
| **Total** | **12V** | **~2.5A** | **~25W** |

### Power Supply Sizing
- **Recommended**: 12V 3A (36W) power supply
- **Margin**: 44% power overhead for safety
- **Regulation**: 12V → 5V → 3.3V via onboard regulators

## Signal Integrity Considerations

### I2S Bus Layout
```
ESP32 Pin 22 (SCK) ──── INMP441 SCK
ESP32 Pin 25 (WS)  ──── INMP441 WS  
ESP32 Pin 26 (SD)  ──── INMP441 SD
```
- Keep I2S traces short (<10cm recommended)
- Parallel routing for clock integrity
- Ground plane for noise reduction

### Audio Output Path
```
ESP32 Pin 21 ───[1kΩ]─── PAM8610 L_IN
                         PAM8610 R_IN (tied together for mono)
```
- 1kΩ series resistor protects ESP32 GPIO
- Short audio lines to minimize noise pickup
- Keep away from switching power supply traces

## Timing Analysis

### I2S Clock Domain
- Sample Rate: 16 kHz
- Bit Depth: 16-bit
- Clock Rate: 16,000 × 16 × 2 = 512 kHz
- Well within ESP32 I2S capabilities (up to several MHz)

### Audio Processing Load
- Buffer Size: 1024 samples
- Processing Interval: 100ms
- CPU Load: <5% estimated for basic voice detection

## Environmental Considerations

### EMI/RFI Mitigation
- WiFi antenna away from audio circuits
- Power supply filtering
- Ground plane design
- Shielded audio cables if needed

### Thermal Management
- PAM8610 may need heatsink at high power
- ESP32 temperature monitoring available
- Adequate ventilation around amplifier

## Testing Checklist

### ✅ Hardware Validation
- [ ] Verify all pin connections before power-on
- [ ] Test with 5V first, then 12V
- [ ] Check continuity and no shorts
- [ ] Measure supply voltages at each component

### ✅ Software Validation
- [ ] I2S initialization successful
- [ ] Audio input data reception
- [ ] PWM audio output generation
- [ ] MQTT functionality preserved
- [ ] Voice command recognition

### ✅ System Integration
- [ ] No interference between MQTT and audio
- [ ] Relay control unaffected by audio processing
- [ ] Power consumption within limits
- [ ] Stable operation over time

## Conclusion

✅ **Pin assignment is VALID and SAFE**
- No GPIO conflicts
- Appropriate pin capabilities utilized
- Existing functionality preserved
- Good signal integrity design
- Adequate power budget

The configuration is ready for implementation with the provided wiring diagram and firmware.


