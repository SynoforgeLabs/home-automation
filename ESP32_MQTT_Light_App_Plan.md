# ESP32 MQTT Light Controller - React Native App Development Plan

## Project Overview

Build a React Native app using Expo SDK 53 that controls an ESP32-based MQTT light relay with Siri and Google Assistant integration.

## Current ESP32 Setup

- **Hardware**: ESP32 + Single Relay Module
- **GPIO Pin**: 4 (LIGHT_RELAY_PIN)
- **MQTT Broker**: 192.168.1.7:1883
- **MQTT Topics**: 
  - Control: `home/light` (payload: "on"/"off")
  - Status: `home/light/status`
- **Credentials**: testuser:5362

## App Requirements

### Core Features
- [x] Basic UI with ON/OFF buttons
- [x] MQTT client for ESP32 communication
- [x] Real-time status updates
- [x] Siri integration (iOS)
- [x] Google Assistant integration (Android)

### Technical Stack
- **Framework**: React Native with Expo SDK 53
- **MQTT Library**: Paho MQTT for React Native
- **iOS Voice**: Siri Shortcuts
- **Android Voice**: App Actions
- **State Management**: React Hooks (useState/useEffect)

---

## Phase 1: Project Setup

### 1.1 Initialize Expo Project

```bash
# Create new Expo project
npx create-expo-app@latest ESP32LightController --template blank-typescript

# Navigate to project
cd ESP32LightController

# Install dependencies
npx expo install expo-dev-client
npm install paho-mqtt
npm install @expo/vector-icons
```

### 1.2 Configure app.json

```json
{
  "expo": {
    "name": "ESP32 Light Controller",
    "slug": "esp32-light-controller",
    "version": "1.0.0",
    "orientation": "portrait",
    "icon": "./assets/icon.png",
    "userInterfaceStyle": "light",
    "splash": {
      "image": "./assets/splash.png",
      "resizeMode": "contain",
      "backgroundColor": "#ffffff"
    },
    "assetBundlePatterns": ["**/*"],
    "ios": {
      "supportsTablet": true,
      "bundleIdentifier": "com.yourname.esp32lightcontroller",
      "infoPlist": {
        "NSUserActivityTypes": ["com.yourname.esp32lightcontroller.toggle-light"]
      }
    },
    "android": {
      "adaptiveIcon": {
        "foregroundImage": "./assets/adaptive-icon.png",
        "backgroundColor": "#FFFFFF"
      },
      "package": "com.yourname.esp32lightcontroller",
      "intentFilters": [
        {
          "action": "VIEW",
          "category": ["DEFAULT", "BROWSABLE"],
          "data": {
            "scheme": "esp32light"
          }
        }
      ]
    },
    "web": {
      "favicon": "./assets/favicon.png"
    },
    "plugins": [
      "expo-dev-client"
    ]
  }
}
```

---

## Phase 2: Core App Development

### 2.1 Main App Component (App.tsx)

```typescript
import React, { useState, useEffect } from 'react';
import {
  StyleSheet,
  Text,
  View,
  TouchableOpacity,
  Alert,
  StatusBar,
  ActivityIndicator,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import MQTTClient from './services/MQTTClient';

export default function App() {
  const [lightState, setLightState] = useState<'on' | 'off'>('off');
  const [connectionStatus, setConnectionStatus] = useState<'disconnected' | 'connecting' | 'connected'>('disconnected');
  const [mqttClient, setMqttClient] = useState<MQTTClient | null>(null);

  useEffect(() => {
    initializeMQTT();
    return () => {
      if (mqttClient) {
        mqttClient.disconnect();
      }
    };
  }, []);

  const initializeMQTT = async () => {
    try {
      setConnectionStatus('connecting');
      const client = new MQTTClient({
        host: '192.168.1.7',
        port: 1883,
        username: 'testuser',
        password: '5362',
        onStatusUpdate: (status: 'on' | 'off') => {
          setLightState(status);
        },
        onConnectionChange: (connected: boolean) => {
          setConnectionStatus(connected ? 'connected' : 'disconnected');
        },
      });

      await client.connect();
      setMqttClient(client);
    } catch (error) {
      console.error('MQTT Connection failed:', error);
      setConnectionStatus('disconnected');
      Alert.alert('Connection Error', 'Failed to connect to ESP32. Please check your WiFi connection.');
    }
  };

  const toggleLight = async () => {
    if (!mqttClient || connectionStatus !== 'connected') {
      Alert.alert('Not Connected', 'Please wait for MQTT connection...');
      return;
    }

    const newState = lightState === 'on' ? 'off' : 'on';
    try {
      await mqttClient.controlLight(newState);
    } catch (error) {
      Alert.alert('Control Error', 'Failed to control light');
    }
  };

  const getStatusColor = () => {
    switch (connectionStatus) {
      case 'connected': return '#4CAF50';
      case 'connecting': return '#FF9800';
      default: return '#F44336';
    }
  };

  const getLightColor = () => {
    return lightState === 'on' ? '#FFC107' : '#9E9E9E';
  };

  return (
    <View style={styles.container}>
      <StatusBar barStyle="dark-content" />
      
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.title}>ESP32 Light Controller</Text>
        <View style={[styles.statusIndicator, { backgroundColor: getStatusColor() }]}>
          <Text style={styles.statusText}>
            {connectionStatus === 'connecting' && <ActivityIndicator size="small" color="white" />}
            {connectionStatus}
          </Text>
        </View>
      </View>

      {/* Light Status */}
      <View style={styles.lightContainer}>
        <Ionicons 
          name="bulb" 
          size={120} 
          color={getLightColor()} 
          style={styles.lightIcon}
        />
        <Text style={styles.lightStatus}>
          Light is {lightState.toUpperCase()}
        </Text>
      </View>

      {/* Control Button */}
      <TouchableOpacity
        style={[
          styles.controlButton,
          { backgroundColor: lightState === 'on' ? '#F44336' : '#4CAF50' }
        ]}
        onPress={toggleLight}
        disabled={connectionStatus !== 'connected'}
      >
        <Ionicons 
          name={lightState === 'on' ? 'power' : 'power'} 
          size={24} 
          color="white" 
        />
        <Text style={styles.buttonText}>
          Turn {lightState === 'on' ? 'OFF' : 'ON'}
        </Text>
      </TouchableOpacity>

      {/* Retry Connection */}
      {connectionStatus === 'disconnected' && (
        <TouchableOpacity style={styles.retryButton} onPress={initializeMQTT}>
          <Text style={styles.retryText}>Retry Connection</Text>
        </TouchableOpacity>
      )}

      {/* Voice Commands Info */}
      <View style={styles.voiceInfo}>
        <Text style={styles.voiceTitle}>Voice Commands:</Text>
        <Text style={styles.voiceCommand}>📱 iOS: "Hey Siri, toggle living room light"</Text>
        <Text style={styles.voiceCommand}>🤖 Android: "Hey Google, turn on the light"</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
    paddingTop: 50,
  },
  header: {
    alignItems: 'center',
    marginBottom: 40,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 10,
  },
  statusIndicator: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 15,
    flexDirection: 'row',
    alignItems: 'center',
  },
  statusText: {
    color: 'white',
    fontWeight: 'bold',
    fontSize: 12,
    textTransform: 'uppercase',
  },
  lightContainer: {
    alignItems: 'center',
    marginVertical: 40,
  },
  lightIcon: {
    marginBottom: 20,
  },
  lightStatus: {
    fontSize: 18,
    fontWeight: '600',
    color: '#333',
  },
  controlButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    marginHorizontal: 40,
    paddingVertical: 15,
    borderRadius: 10,
    elevation: 3,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.25,
    shadowRadius: 3.84,
  },
  buttonText: {
    color: 'white',
    fontSize: 18,
    fontWeight: 'bold',
    marginLeft: 10,
  },
  retryButton: {
    marginTop: 20,
    marginHorizontal: 40,
    paddingVertical: 12,
    backgroundColor: '#2196F3',
    borderRadius: 8,
    alignItems: 'center',
  },
  retryText: {
    color: 'white',
    fontWeight: 'bold',
  },
  voiceInfo: {
    position: 'absolute',
    bottom: 50,
    left: 20,
    right: 20,
    backgroundColor: 'white',
    padding: 15,
    borderRadius: 10,
    elevation: 2,
  },
  voiceTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 8,
    color: '#333',
  },
  voiceCommand: {
    fontSize: 14,
    color: '#666',
    marginBottom: 4,
  },
});
```

### 2.2 MQTT Client Service (services/MQTTClient.ts)

```typescript
import { Client as PahoClient } from 'paho-mqtt';

interface MQTTConfig {
  host: string;
  port: number;
  username: string;
  password: string;
  onStatusUpdate: (status: 'on' | 'off') => void;
  onConnectionChange: (connected: boolean) => void;
}

export default class MQTTClient {
  private client: PahoClient;
  private config: MQTTConfig;
  private isConnected: boolean = false;

  constructor(config: MQTTConfig) {
    this.config = config;
    this.client = new PahoClient(
      config.host,
      config.port,
      `ESP32App_${Math.random().toString(36).substr(2, 9)}`
    );

    this.setupEventHandlers();
  }

  private setupEventHandlers() {
    this.client.onConnectionLost = (responseObject) => {
      console.log('MQTT Connection lost:', responseObject.errorMessage);
      this.isConnected = false;
      this.config.onConnectionChange(false);
    };

    this.client.onMessageArrived = (message) => {
      console.log('MQTT Message arrived:', message.destinationName, message.payloadString);
      
      if (message.destinationName === 'home/light/status') {
        const status = message.payloadString as 'on' | 'off';
        this.config.onStatusUpdate(status);
      }
    };
  }

  async connect(): Promise<void> {
    return new Promise((resolve, reject) => {
      this.client.connect({
        userName: this.config.username,
        password: this.config.password,
        onSuccess: () => {
          console.log('MQTT Connected successfully');
          this.isConnected = true;
          this.config.onConnectionChange(true);
          
          // Subscribe to status updates
          this.client.subscribe('home/light/status');
          
          // Request current status
          this.publishMessage('home/light/get', 'status');
          
          resolve();
        },
        onFailure: (error) => {
          console.error('MQTT Connection failed:', error);
          this.isConnected = false;
          this.config.onConnectionChange(false);
          reject(error);
        },
      });
    });
  }

  async controlLight(state: 'on' | 'off'): Promise<void> {
    if (!this.isConnected) {
      throw new Error('MQTT not connected');
    }

    this.publishMessage('home/light', state);
  }

  private publishMessage(topic: string, payload: string) {
    const message = new PahoClient.Message(payload);
    message.destinationName = topic;
    this.client.send(message);
    console.log(`MQTT Published: ${topic} -> ${payload}`);
  }

  disconnect() {
    if (this.isConnected) {
      this.client.disconnect();
      this.isConnected = false;
      this.config.onConnectionChange(false);
    }
  }
}
```

---

## Phase 3: iOS Siri Integration

### 3.1 Install Siri Shortcuts Library

```bash
npm install react-native-siri-shortcut
```

### 3.2 Create Siri Integration Service (services/SiriIntegration.ts)

```typescript
import { SiriShortcutsEvent } from 'react-native-siri-shortcut';
import { Platform } from 'react-native';

interface SiriShortcutConfig {
  onVoiceCommand: (action: 'on' | 'off') => void;
}

export class SiriIntegration {
  private config: SiriShortcutConfig;

  constructor(config: SiriShortcutConfig) {
    this.config = config;
    this.setupSiriListener();
  }

  private setupSiriListener() {
    if (Platform.OS === 'ios') {
      SiriShortcutsEvent.addListener('SiriShortcutListener', ({ userInfo }) => {
        console.log('Siri shortcut triggered:', userInfo);
        
        if (userInfo?.action === 'light_on') {
          this.config.onVoiceCommand('on');
        } else if (userInfo?.action === 'light_off') {
          this.config.onVoiceCommand('off');
        }
      });
    }
  }

  async setupShortcuts() {
    if (Platform.OS !== 'ios') {
      console.log('Siri shortcuts only available on iOS');
      return;
    }

    try {
      // Turn ON shortcut
      await SiriShortcutsEvent.donateShortcut({
        activityType: 'com.yourname.esp32lightcontroller.light-on',
        title: 'Turn On Living Room Light',
        userInfo: { action: 'light_on' },
        keywords: ['light', 'on', 'living room', 'turn on'],
        persistentIdentifier: 'light_on',
        isEligibleForSearch: true,
        isEligibleForPrediction: true,
        suggestedInvocationPhrase: 'Turn on living room light',
      });

      // Turn OFF shortcut
      await SiriShortcutsEvent.donateShortcut({
        activityType: 'com.yourname.esp32lightcontroller.light-off',
        title: 'Turn Off Living Room Light',
        userInfo: { action: 'light_off' },
        keywords: ['light', 'off', 'living room', 'turn off'],
        persistentIdentifier: 'light_off',
        isEligibleForSearch: true,
        isEligibleForPrediction: true,
        suggestedInvocationPhrase: 'Turn off living room light',
      });

      console.log('Siri shortcuts registered successfully');
    } catch (error) {
      console.error('Failed to register Siri shortcuts:', error);
    }
  }
}
```

### 3.3 iOS Info.plist Configuration

Add to `ios/ESP32LightController/Info.plist`:

```xml
<key>NSUserActivityTypes</key>
<array>
  <string>com.yourname.esp32lightcontroller.light-on</string>
  <string>com.yourname.esp32lightcontroller.light-off</string>
</array>
```

---

## Phase 4: Android Google Assistant Integration

### 4.1 Create App Actions Configuration

Create `android/app/src/main/res/xml/shortcuts.xml`:

```xml
<shortcuts xmlns:android="http://schemas.android.com/apk/res/android">
  <capability android:name="actions.intent.TURN_ON">
    <intent
      android:action="android.intent.action.VIEW"
      android:targetPackage="com.yourname.esp32lightcontroller"
      android:targetClass="com.yourname.esp32lightcontroller.MainActivity">
      <parameter
        android:name="actions.intent.extra.ITEM"
        android:key="item">
        <entity-set-reference android:entitySetId="LightEntitySet"/>
      </parameter>
    </intent>
  </capability>
  
  <capability android:name="actions.intent.TURN_OFF">
    <intent
      android:action="android.intent.action.VIEW"
      android:targetPackage="com.yourname.esp32lightcontroller"
      android:targetClass="com.yourname.esp32lightcontroller.MainActivity">
      <parameter
        android:name="actions.intent.extra.ITEM"
        android:key="item">
        <entity-set-reference android:entitySetId="LightEntitySet"/>
      </parameter>
    </intent>
  </capability>
  
  <entity-set android:entitySetId="LightEntitySet">
    <entity
      android:name="living room light"
      android:alternateName="@array/living_room_light_synonyms"
      android:identifier="living_room_light"/>
  </entity-set>
</shortcuts>
```

### 4.2 Create String Resources

Create `android/app/src/main/res/values/arrays.xml`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<resources>
  <string-array name="living_room_light_synonyms">
    <item>light</item>
    <item>lights</item>
    <item>living room light</item>
    <item>main light</item>
  </string-array>
</resources>
```

### 4.3 Update AndroidManifest.xml

Add to `android/app/src/main/AndroidManifest.xml`:

```xml
<activity
  android:name=".MainActivity"
  android:exported="true"
  android:launchMode="singleTop"
  android:theme="@style/Theme.App.SplashScreen">
  
  <intent-filter>
    <action android:name="android.intent.action.MAIN" />
    <category android:name="android.intent.category.LAUNCHER" />
  </intent-filter>
  
  <!-- App Actions Intent Filters -->
  <intent-filter>
    <action android:name="android.intent.action.VIEW" />
    <category android:name="android.intent.category.DEFAULT" />
    <category android:name="android.intent.category.BROWSABLE" />
    <data android:scheme="esp32light" />
  </intent-filter>
  
  <meta-data
    android:name="android.app.shortcuts"
    android:resource="@xml/shortcuts" />
</activity>
```

### 4.4 Android Deep Link Handler

Add to your main App.tsx:

```typescript
import { Linking } from 'react-native';

// Add this useEffect in your App component
useEffect(() => {
  const handleDeepLink = (url: string) => {
    console.log('Deep link received:', url);
    
    if (url.includes('light_on') || url.includes('TURN_ON')) {
      toggleLightTo('on');
    } else if (url.includes('light_off') || url.includes('TURN_OFF')) {
      toggleLightTo('off');
    }
  };

  // Check initial URL
  Linking.getInitialURL().then((url) => {
    if (url) {
      handleDeepLink(url);
    }
  });

  // Listen for incoming URLs
  const subscription = Linking.addEventListener('url', (event) => {
    handleDeepLink(event.url);
  });

  return () => subscription?.remove();
}, []);

const toggleLightTo = async (state: 'on' | 'off') => {
  if (mqttClient && connectionStatus === 'connected') {
    try {
      await mqttClient.controlLight(state);
    } catch (error) {
      console.error('Voice command failed:', error);
    }
  }
};
```

---

## Phase 5: Testing & Deployment

### 5.1 Development Testing

```bash
# Start development server
npx expo start

# Test on physical devices
npx expo run:ios
npx expo run:android
```

### 5.2 Voice Assistant Testing

**iOS Testing:**
1. Build app on physical iPhone
2. Use the app to register Siri shortcuts
3. Say: "Hey Siri, turn on living room light"
4. Verify Siri shortcuts appear in Settings > Siri & Search

**Android Testing:**
1. Build app on physical Android device
2. Test Google Assistant: "Hey Google, turn on living room light"
3. Use Google Assistant app to verify app actions

### 5.3 ESP32 Testing Commands

```bash
# Test MQTT connection
mosquitto_pub -h 192.168.1.7 -p 1883 -u testuser -P 5362 -t "home/light" -m "on"
mosquitto_pub -h 192.168.1.7 -p 1883 -u testuser -P 5362 -t "home/light" -m "off"

# Monitor status
mosquitto_sub -h 192.168.1.7 -p 1883 -u testuser -P 5362 -t "home/light/status"
```

---

## Phase 6: Build for Production

### 6.1 Create Production Build

```bash
# Build for iOS (requires Mac and Xcode)
npx expo run:ios --configuration Release

# Build for Android
npx expo build:android --type apk

# Or use EAS Build (recommended)
npm install -g @expo/eas-cli
eas login
eas build --platform all
```

### 6.2 App Store Submission (Optional)

Follow Expo documentation for App Store and Google Play Store submissions.

---

## Project Structure

```
ESP32LightController/
├── App.tsx                          # Main app component
├── app.json                         # Expo configuration
├── package.json                     # Dependencies
├── services/
│   ├── MQTTClient.ts               # MQTT client service
│   └── SiriIntegration.ts          # Siri shortcuts service
├── android/
│   └── app/src/main/
│       ├── AndroidManifest.xml     # Android permissions
│       └── res/
│           ├── xml/shortcuts.xml   # App Actions config
│           └── values/arrays.xml   # String resources
└── ios/
    └── ESP32LightController/
        └── Info.plist              # iOS configuration
```

---

## Voice Commands Summary

| Platform | Command | Action |
|----------|---------|--------|
| iOS Siri | "Hey Siri, turn on living room light" | Turns light ON |
| iOS Siri | "Hey Siri, turn off living room light" | Turns light OFF |
| Android | "Hey Google, turn on the light" | Turns light ON |
| Android | "Hey Google, turn off the light" | Turns light OFF |

---

## Troubleshooting

### Common Issues

1. **MQTT Connection Fails**
   - Check WiFi connection
   - Verify ESP32 IP address
   - Ensure MQTT broker is running

2. **Siri Shortcuts Not Working**
   - Rebuild app in development mode
   - Check iOS device settings for Siri shortcuts
   - Verify NSUserActivityTypes in Info.plist

3. **Google Assistant Not Responding**
   - Check AndroidManifest.xml configuration
   - Verify shortcuts.xml is properly formatted
   - Test deep links manually

4. **App Crashes on Voice Command**
   - Check console logs
   - Ensure MQTT client is connected
   - Add proper error handling

---

## Next Steps & Enhancements

1. **Multiple Devices**: Extend to control multiple relays
2. **Scheduling**: Add timer-based control
3. **Automation**: Add sensor-based triggers
4. **UI Improvements**: Add themes and animations
5. **Cloud Sync**: Add cloud MQTT broker support
6. **Push Notifications**: Add status notifications

---

## Security Considerations

1. **MQTT Security**: Use SSL/TLS for production
2. **Network Security**: Consider VPN for remote access
3. **App Security**: Add authentication for sensitive controls
4. **Voice Security**: Implement confirmation for critical commands

---

**Ready to build! This plan provides everything needed to create a working React Native app with voice assistant integration for your ESP32 MQTT light controller.**
