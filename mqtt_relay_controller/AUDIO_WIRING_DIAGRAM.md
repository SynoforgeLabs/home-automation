# ESP32 Audio-Enabled Light Controller Wiring Diagram

## Component Overview
- **ESP32 Dev Board** (Main controller)
- **Relay Module** (Light control - existing)
- **INMP441 Microphone** (Voice input)
- **PAM8610 Amplifier** (Audio output)
- **8Ω 10W Speaker** (Audio feedback)

## Pin Assignments

### ESP32 GPIO Usage (Updated Pin Assignments - Mono Audio)
| GPIO | Function | Component | Notes |
|------|----------|-----------|-------|
| GPIO 4 | Relay Control | Light Relay | Existing - HIGH = ON |
| GPIO 22 | I2S SCK | INMP441 | Serial Clock |
| **GPIO 32** | **I2S WS** | **INMP441** | **Word Select (L/R) - FIXED CONFLICT** |
| GPIO 26 | I2S SD | INMP441 | Serial Data |
| **GPIO 19** | **Audio Out** | **PAM8610** | **Right Channel Audio (Mono Setup)** |
| GPIO 18 | Enable | PAM8610 | Amplifier Enable (Optional) |
| 3.3V | Power | INMP441 | Microphone Power |
| 5V/VIN | Power | PAM8610 | Amplifier Power (12V recommended) |

**📝 NOTES:** 
- **GPIO 25 conflict fixed:** Changed I2S WS pin to GPIO 32
- **Mono audio setup:** Using only right channel (GPIO 19) for single speaker
- **GPIO 21 freed up:** Can be used for other purposes

## Detailed Wiring Connections

### 1. INMP441 Microphone Module (I2S Interface) - UPDATED
```
INMP441     →   ESP32
VCC         →   3.3V
GND         →   GND
WS          →   GPIO 32 (Word Select) ⭐ CHANGED FROM GPIO 25
SCK         →   GPIO 22 (Serial Clock)
SD          →   GPIO 26 (Serial Data Out)
``` 

### 2. PAM8610 Amplifier Board (Mono Setup - Single Speaker)
```
PAM8610     →   Connection
VCC         →   12V Power Supply (6-15V range)
GND         →   Common Ground
L_IN        →   Not Connected (mono right channel setup)
R_IN        →   GPIO 19 (via 1kΩ resistor) - Mono audio input
EN          →   GPIO 18 (amplifier enable - optional)
L_OUT+      →   Not Connected (mono setup)
L_OUT-      →   Not Connected (mono setup)
R_OUT+      →   Speaker Positive
R_OUT-      →   Speaker Negative
```

### 3. 8Ω 10W Speaker (Single Speaker Setup)
```
Speaker     →   PAM8610
Positive    →   R_OUT+ (right channel output)
Negative    →   R_OUT- (right channel output)
```

### 4. Existing Light Relay
```
Relay       →   ESP32
VCC         →   5V (or 3.3V depending on relay)
GND         →   GND
IN          →   GPIO 4
COM         →   AC Live/Neutral
NO          →   Light Circuit
```

## Power Supply Requirements

### Option 1: Single 12V Supply
- **12V 3A Power Supply** (recommended)
  - ESP32: 12V → 5V regulator → 3.3V onboard
  - PAM8610: 12V direct
  - Relay: 5V from ESP32 board
  - INMP441: 3.3V from ESP32

### Option 2: Dual Supply
- **5V 2A** for ESP32 and peripherals
- **12V 3A** for PAM8610 amplifier

## Audio Signal Path (Mono Setup)
```
Voice → INMP441 → I2S → ESP32 → Processing → GPIO19 → PAM8610 R_IN → R_OUT → Speaker
```

## Important Notes

### Power Considerations
- PAM8610 can consume up to 3A at 12V for full 15W output
- At 10W (speaker rating), expect ~2A current draw
- Use adequate heatsink for PAM8610 at higher power levels

### Audio Quality
- Use short, shielded wires for audio signals
- Keep audio lines away from power lines to reduce noise
- 1kΩ series resistors protect ESP32 GPIO from amplifier input

### Grounding
- **Critical**: Common ground for all components
- Use star grounding configuration if possible
- Separate analog and digital grounds if using mixed signals

### GPIO Protection
- All ESP32 GPIOs are 3.3V tolerant
- Use current limiting resistors for audio output
- Pull-up/pull-down resistors may be needed for stable operation

## Physical Layout Recommendations (Mono Setup)
```
[12V PSU] ──┬── [PAM8610] ── [Single Speaker]
            │       │
            │       └── GPIO19 (R_IN)
            │
            ├── [ESP32 Board]
            │        │
            │        ├── [GPIO4] ── [Relay] ── [Light]
            │        │
            │        ├── [GPIO32,22,26] ── [INMP441 Microphone]
            │        │
            │        └── [GPIO18] ── [PAM8610 Enable]
            │
            └── [Common GND]
```

**Simplified Mono Wiring:**
- Only need to connect GPIO 19 to PAM8610 R_IN
- Single speaker connects to R_OUT+ and R_OUT-
- GPIO 21 is now free for other uses

## Testing Checklist
- [ ] Verify all connections before power-on
- [ ] Test with 5V first, then 12V
- [ ] Check audio output with simple tone
- [ ] Verify I2S microphone data capture
- [ ] Test relay operation (existing functionality)
- [ ] Monitor current consumption
- [ ] Check for overheating in amplifier

## Safety Notes
- Never exceed 15V on PAM8610 input
- Ensure proper speaker impedance (4-8Ω)
- Use appropriate fuses for high-current circuits
- Verify all connections before applying power
- Keep amplifier away from moisture


