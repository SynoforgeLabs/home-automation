# ESP32 Light Control Wiring Guide

This guide explains how to safely wire an ESP32 microcontroller to control a light bulb using a relay module.

## ⚠️ SAFETY WARNINGS

**ELECTRICAL SAFETY IS CRITICAL - READ CAREFULLY BEFORE PROCEEDING**

- **TURN OFF MAIN POWER** at the circuit breaker before making any connections
- **NEVER work on live circuits** - AC mains voltage can be LETHAL
- **Use proper electrical enclosures** to prevent accidental contact with live wires
- **Double-check all connections** before applying power
- **Use appropriate wire gauges** for the current load
- **Install proper fuses/circuit breakers** for protection
- **If unsure, consult a qualified electrician**

## Required Components

### Hardware
- **ESP32 Development Board** (any variant with WiFi)
- **Single Channel Relay Module** (5V or 3.3V compatible)
- **Light Bulb** (LED or incandescent, appropriate wattage)
- **Light Fixture/Socket**
- **Jumper Wires** (Male-to-Female)
- **Breadboard** (optional, for prototyping)
- **Electrical Wire** (14 AWG for household lighting)
- **Wire Nuts** or **Terminal Blocks**
- **Electrical Enclosure Box**

### Tools
- Wire strippers
- Screwdrivers
- Multimeter
- Wire nuts or terminal blocks

## Component Overview

### ESP32 Pinout
```
ESP32 Development Board
┌─────────────────────┐
│  3V3    EN     D23  │
│  GND    VP     D22  │
│  D15    VN     TX0  │
│  D2     D34    RX0  │
│  D4  ←──D35    D21  │  ← GPIO 4 (LIGHT_RELAY_PIN)
│  D16    D32    GND  │
│  D17    D33    D19  │
│  D5     D25    D18  │
│  D18    D26    D5   │
│  D19    D27    TX2  │
│  D21    D14    RX2  │
│  RX2    D12    D4   │
│  TX2    D13    D2   │
│  D22    GND    D15  │
│  D23    VIN    GND  │
└─────────────────────┘
```

### Relay Module
```
Single Channel Relay Module
┌─────────────────────┐
│ VCC  IN   GND       │  ← Control pins
│  │    │    │        │
│  │    │    │        │
│ ┌────────────┐      │
│ │   RELAY    │      │
│ └────────────┘      │
│                     │
│ COM  NO   NC        │  ← Switch contacts
└─────────────────────┘

VCC: Power (3.3V or 5V)
IN:  Control signal from ESP32
GND: Ground
COM: Common terminal
NO:  Normally Open contact
NC:  Normally Closed contact
```

## Wiring Connections

### Step 1: ESP32 to Relay Module (Low Voltage Side)

**SAFE TO WORK ON - LOW VOLTAGE (3.3V/5V)**

| ESP32 Pin | Relay Module Pin | Wire Color (Suggested) |
|-----------|------------------|------------------------|
| 3V3       | VCC              | Red                    |
| GND       | GND              | Black                  |
| GPIO 4    | IN               | Yellow                 |

```
ESP32                    Relay Module
┌─────────┐             ┌─────────────┐
│   3V3   │────────────▶│    VCC      │
│   GND   │────────────▶│    GND      │
│ GPIO 4  │────────────▶│     IN      │
└─────────┘             └─────────────┘
```

### Step 2: AC Mains Wiring (High Voltage Side)

**⚠️ DANGER - HIGH VOLTAGE - TURN OFF POWER FIRST**

```
Main Power Supply        Relay Module           Light Fixture
┌─────────────────┐     ┌─────────────────┐    ┌─────────────┐
│                 │     │                 │    │             │
│   HOT (Black)   │────▶│      COM        │    │             │
│                 │     │                 │    │             │
│ NEUTRAL (White) │─────┼─────────────────┼───▶│   NEUTRAL   │
│                 │     │                 │    │   (White)   │
│  GROUND (Green) │─────┼─────────────────┼───▶│   GROUND    │
│                 │     │       NO        │───▶│    HOT      │
└─────────────────┘     │                 │    │  (Black)    │
                        └─────────────────┘    └─────────────┘
```

**Detailed AC Wiring Steps:**

1. **Turn OFF the main circuit breaker**
2. **Verify power is off** with a multimeter
3. Connect the **HOT wire (black)** from the main supply to the **COM** terminal of the relay
4. Connect the **NO (Normally Open)** terminal of the relay to the **HOT wire** of the light fixture
5. Connect the **NEUTRAL wire (white)** from main supply directly to the **NEUTRAL** of the light fixture
6. Connect the **GROUND wire (green)** from main supply directly to the **GROUND** of the light fixture

### Complete System Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                          COMPLETE SYSTEM                        │
└─────────────────────────────────────────────────────────────────┘

    12V Power Supply         ESP32 Board              Relay Module
   ┌─────────────────┐      ┌─────────────┐         ┌─────────────────┐
   │       +12V      │─────▶│     VIN     │         │                 │
   │       GND       │─────▶│     GND     │───┬────▶│      GND        │
   └─────────────────┘      │             │   │     │                 │
                            │   GPIO 4    │───┼────▶│       IN        │
                            │     3V3     │───┼────▶│      VCC        │
                            └─────────────┘   │     └─────────────────┘
                                              │              │
    AC Mains (220V/110V)                      │              ▼
   ┌─────────────────┐                        │     ┌─────────────────┐
   │   HOT (Black)   │────────────────────────┼────▶│      COM        │
   │ NEUTRAL (White) │────────────────────────┼─────┼─────────────────┼──┐
   │ GROUND (Green)  │────────────────────────┘     │       NO        │  │
   └─────────────────┘                              └─────────────────┘  │
                                                             │            │
                                                             ▼            ▼
                                                    ┌─────────────────────────┐
                                                    │     Light Fixture       │
                                                    │                         │
                                                    │  HOT ──────────────────▲│
                                                    │  NEUTRAL ──────────────▲│
                                                    │  GROUND ────────────────┘│
                                                    └─────────────────────────┘
```

## Software Configuration

The ESP32 code is already configured for this setup:

- **GPIO Pin**: 4 (LIGHT_RELAY_PIN)
- **MQTT Topic**: `home/light` (for control)
- **Status Topic**: `home/light/status` (for status updates)
- **Relay Logic**: 
  - `LOW` signal = Relay ON = Light ON
  - `HIGH` signal = Relay OFF = Light OFF

## Testing Procedure

### 1. Pre-Power Testing

**Before connecting AC power:**

1. **Verify all low-voltage connections** (ESP32 to relay)
2. **Upload the code** to ESP32
3. **Check serial monitor** for WiFi and MQTT connection
4. **Test relay clicking** by sending MQTT commands:
   ```bash
   mosquitto_pub -h 192.168.1.7 -p 1883 -u testuser -P 5362 -t "home/light" -m "on"
   mosquitto_pub -h 192.168.1.7 -p 1883 -u testuser -P 5362 -t "home/light" -m "off"
   ```
5. **Listen for the relay clicking sound** when commands are sent

### 2. AC Power Testing

**After verifying low-voltage operation:**

1. **Double-check all AC connections**
2. **Ensure proper electrical enclosure**
3. **Turn ON the main circuit breaker**
4. **Test light control** via MQTT:
   ```bash
   # Turn light ON
   mosquitto_pub -h 192.168.1.7 -p 1883 -u testuser -P 5362 -t "home/light" -m "on"
   
   # Turn light OFF
   mosquitto_pub -h 192.168.1.7 -p 1883 -u testuser -P 5362 -t "home/light" -m "off"
   ```

### 3. Monitor Status

Subscribe to status updates:
```bash
mosquitto_sub -h 192.168.1.7 -p 1883 -u testuser -P 5362 -t "home/light/status"
```

## Troubleshooting

### Common Issues

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| ESP32 won't connect to WiFi | Wrong credentials | Check SSID/password in code |
| MQTT connection fails | Wrong broker IP/credentials | Verify broker IP and user/password |
| Relay doesn't click | Wiring issue | Check ESP32 to relay connections |
| Light doesn't turn on | AC wiring issue | Verify AC connections (POWER OFF FIRST) |
| Relay clicks but no light | Faulty bulb or fixture | Test bulb/fixture separately |

### Serial Monitor Output

Expected serial output when working correctly:
```
Connecting to SLT-Fiber-EYcM6-2.4G
..........
WiFi connected.
IP address: 192.168.1.xxx
Connecting to MQTT...
MQTT connected successfully
Subscribed to home/light topic
Published initial light state: off
```

### Safety Checklist

- [ ] Main power is OFF during AC wiring
- [ ] All connections are secure
- [ ] Proper wire gauges are used
- [ ] Electrical enclosure is properly closed
- [ ] No exposed live wires
- [ ] Circuit protection (fuse/breaker) is in place
- [ ] Ground connections are secure

## Advanced Features

### Home Assistant Integration

Add to your Home Assistant `configuration.yaml`:

```yaml
mqtt:
  light:
    - name: "Living Room Light"
      command_topic: "home/light"
      state_topic: "home/light/status"
      payload_on: "on"
      payload_off: "off"
```

### Voice Control (with Home Assistant + Google/Alexa)

Once integrated with Home Assistant, you can control the light with voice commands:
- "Hey Google, turn on the living room light"
- "Alexa, turn off the living room light"

## Maintenance

- **Regular inspection** of connections
- **Check for loose wires** monthly
- **Monitor ESP32 performance** via serial output
- **Update firmware** as needed
- **Test emergency shutoff** procedures

---

**Remember: When in doubt about electrical work, always consult a qualified electrician. Safety first!**
