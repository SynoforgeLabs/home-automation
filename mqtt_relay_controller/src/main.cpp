#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <driver/i2s.h>
#include <ArduinoOTA.h>

// Forward declarations
void handleTurnOn(String requestId, String source = "mqtt");
void handleTurnOff(String requestId, String source = "mqtt");
void handleGetStatus(String requestId, String source = "mqtt");
void sendRegistration();
void sendStatus(String requestId);
void sendCommandResponse(String command, String requestId, bool success, String error, String source = "mqtt");

// Audio processing functions
void setupI2S();
void setupAudioOutput();
void processAudioInput();
void playTone(int frequency, int duration);
void playConfirmationSound();
void playErrorSound();
bool detectVoiceActivity();
String processVoiceCommand();
void handleVoiceCommand(String command);

// WiFi management
void setup_wifi();
void checkWiFiConnection();

// OTA management
void setupOTA();

// Replace with your network credentials
const char* ssid = "SLT-Fiber-EYcM6-2.4G";  // Network SSID (name)
const char* password = "aqua1483";  // Network password

// MQTT Configuration
const char* mqtt_server = "broker.hivemq.com";  // Free public MQTT broker for testing
const int mqtt_port = 1883;
const char* deviceId = "esp32-light-controller";
const char* deviceName = "Living Room Light";

// MQTT Topics
const char* status_topic = "devices/esp32-light-controller/status";
const char* heartbeat_topic = "devices/esp32-light-controller/heartbeat";
const char* command_topic = "devices/esp32-light-controller/commands";
const char* response_topic = "devices/esp32-light-controller/responses";
const char* audio_topic = "devices/esp32-light-controller/audio";

// MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

// Variables to store the current state of the light (ON/OFF)
String lightState = "off";

// Pin Assignments (Fixed pin conflicts)
const int LIGHT_RELAY_PIN = 4;   // GPIO 4 for the light relay (existing)

// I2S Microphone Pins (INMP441) - Fixed GPIO 25 conflict
const int I2S_WS = 32;    // Word Select (LRCLK) - Changed from GPIO 25
const int I2S_SCK = 22;   // Serial Clock (BCLK)
const int I2S_SD = 26;    // Serial Data (DOUT)

// Audio Output Pins (to PAM8610) - Mono setup using right channel only
const int AUDIO_OUTPUT_PIN = 19;  // GPIO 19 to amplifier right input (mono setup)
const int AUDIO_ENABLE_PIN = 18;  // GPIO 18 amplifier enable (optional)

// Audio Configuration
const int SAMPLE_RATE = 16000;
const int BUFFER_SIZE = 1024;
const int DETECTION_THRESHOLD = 2000;  // Voice activity threshold (increased for better detection)

// Audio buffers
int16_t audioBuffer[BUFFER_SIZE];
float audioHistory[32];  // History for better voice detection
int historyIndex = 0;

// Variable to enable or disable state saving
const bool saveState = true;

// EEPROM address to store the state
const int eepromSize = 1;

// Timing variables
unsigned long lastHeartbeat = 0;
unsigned long lastReconnect = 0;
unsigned long lastAudioCheck = 0;
unsigned long lastWiFiCheck = 0;
const unsigned long heartbeatInterval = 15000; // 15 seconds
const unsigned long reconnectInterval = 5000;  // 5 seconds
const unsigned long audioCheckInterval = 50;   // 50ms for audio processing (faster)
const unsigned long wifiCheckInterval = 10000; // 10 seconds WiFi check

// Voice command detection
bool voiceDetectionEnabled = true;
unsigned long lastVoiceActivity = 0;
unsigned long voiceCommandStart = 0;
bool isProcessingVoice = false;
const unsigned long voiceTimeoutMs = 2000;     // 2 seconds timeout for voice commands
const unsigned long voiceCommandWindow = 1500; // 1.5 seconds to capture command

// Enhanced voice command patterns with multiple variations
struct VoiceCommand {
  String patterns[3];  // Multiple patterns per command
  String action;
  int patternCount;
};

VoiceCommand voiceCommands[] = {
  {{"turn on", "light on", "switch on"}, "turn_on", 3},
  {{"turn off", "light off", "switch off"}, "turn_off", 3},
  {{"status", "state", "check"}, "get_status", 3}
};
const int NUM_VOICE_COMMANDS = sizeof(voiceCommands) / sizeof(VoiceCommand);

// Voice processing variables
String currentVoiceBuffer = "";
float voiceEnergyHistory[10];
int energyHistoryIndex = 0;

// Connect to WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi - will retry in main loop");
  }
}

// Check WiFi connection and reconnect if needed
void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected - attempting reconnection...");
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("WiFi reconnected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("");
      Serial.println("WiFi reconnection failed - will retry later");
    }
  }
}

// Setup OTA (Over-The-Air) programming
void setupOTA() {
  // Port defaults to 3232
  ArduinoOTA.setPort(3232);

  // Hostname defaults to esp32-[MAC]
  ArduinoOTA.setHostname("esp32-light-controller");

  // Set OTA password for security (optional but recommended)
  ArduinoOTA.setPassword("lightota2024");  // Change this to your preferred password

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("🔄 OTA Update starting (" + type + ")");
    
    // Stop voice detection during OTA
    voiceDetectionEnabled = false;
    
    // Play update sound
    playTone(1000, 200);
    delay(100);
    playTone(1200, 200);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\n✅ OTA Update completed successfully");
    
    // Play completion sound
    playTone(800, 150);
    delay(100);
    playTone(1000, 150);
    delay(100);
    playTone(1200, 150);
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("📊 OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("❌ OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
    
    // Play error sound
    playErrorSound();
    
    // Re-enable voice detection
    voiceDetectionEnabled = true;
  });

  ArduinoOTA.begin();
  Serial.println("🔄 OTA Ready! You can now upload wirelessly.");
  Serial.println("   Hostname: esp32-light-controller");
  Serial.println("   IP: " + WiFi.localIP().toString());
  Serial.println("   Port: 3232");
  Serial.println("   Password: lightota2024");
}

// Setup I2S for microphone input (fixed pin assignments)
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,  // Now using GPIO 32 (fixed conflict)
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  esp_err_t result = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    Serial.printf("Failed to install I2S driver: %d\n", result);
    voiceDetectionEnabled = false;
    return;
  }

  result = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (result != ESP_OK) {
    Serial.printf("Failed to set I2S pins: %d\n", result);
    voiceDetectionEnabled = false;
    return;
  }

  Serial.println("I2S microphone initialized successfully on pins:");
  Serial.printf("  WS (Word Select): GPIO %d\n", I2S_WS);
  Serial.printf("  SCK (Serial Clock): GPIO %d\n", I2S_SCK);
  Serial.printf("  SD (Serial Data): GPIO %d\n", I2S_SD);
}

// Setup audio output to PAM8610 amplifier (mono configuration)
void setupAudioOutput() {
  // Initialize audio output pins (mono setup - right channel only)
  pinMode(AUDIO_OUTPUT_PIN, OUTPUT);
  pinMode(AUDIO_ENABLE_PIN, OUTPUT);
  
  // Set initial states
  digitalWrite(AUDIO_OUTPUT_PIN, LOW);
  digitalWrite(AUDIO_ENABLE_PIN, HIGH);  // Enable amplifier
  
  Serial.println("🔊 PAM8610 audio output initialized (mono):");
  Serial.printf("  Audio Output: GPIO %d (right channel)\n", AUDIO_OUTPUT_PIN);
  Serial.printf("  Enable Pin: GPIO %d\n", AUDIO_ENABLE_PIN);
  Serial.println("  Configuration: Mono (single speaker)");
}

// Play a simple tone on mono output (right channel)
void playTone(int frequency, int duration) {
  int halfPeriod = 1000000 / frequency / 2;
  int cycles = frequency * duration / 1000;
  
  for (int i = 0; i < cycles; i++) {
    digitalWrite(AUDIO_OUTPUT_PIN, HIGH);
    delayMicroseconds(halfPeriod);
    digitalWrite(AUDIO_OUTPUT_PIN, LOW);
    delayMicroseconds(halfPeriod);
  }
}

// Play confirmation sound (success)
void playConfirmationSound() {
  playTone(800, 150);  // 800Hz for 150ms
  delay(50);
  playTone(1200, 150); // 1200Hz for 150ms
  Serial.println("✓ Played confirmation sound");
}

// Play error sound
void playErrorSound() {
  playTone(400, 250);  // 400Hz for 250ms
  delay(100);
  playTone(300, 250);  // 300Hz for 250ms
  Serial.println("✗ Played error sound");
}

// Play startup sound
void playStartupSound() {
  playTone(600, 100);
  delay(50);
  playTone(800, 100);
  delay(50);
  playTone(1000, 100);
  Serial.println("♪ Played startup sound");
}

// Enhanced voice activity detection with noise filtering
bool detectVoiceActivity() {
  if (!voiceDetectionEnabled) return false;
  
  size_t bytesRead = 0;
  esp_err_t result = i2s_read(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytesRead, 10);
  
  if (result != ESP_OK || bytesRead == 0) {
    return false;
  }

  // Calculate RMS (Root Mean Square) for voice activity detection
  long sum = 0;
  int samples = bytesRead / sizeof(int16_t);
  
  for (int i = 0; i < samples; i++) {
    sum += (long)audioBuffer[i] * audioBuffer[i];
  }
  
  float rms = sqrt((float)sum / samples);
  
  // Store in energy history for better detection
  voiceEnergyHistory[energyHistoryIndex] = rms;
  energyHistoryIndex = (energyHistoryIndex + 1) % 10;
  
  // Calculate average energy over recent history
  float avgEnergy = 0;
  for (int i = 0; i < 10; i++) {
    avgEnergy += voiceEnergyHistory[i];
  }
  avgEnergy /= 10;
  
  // Voice activity detected if current RMS is significantly above average
  bool voiceDetected = (rms > DETECTION_THRESHOLD) && (rms > avgEnergy * 1.5);
  
  if (voiceDetected) {
    lastVoiceActivity = millis();
    if (!isProcessingVoice) {
      Serial.printf("Voice activity detected! RMS: %.1f, Avg: %.1f\n", rms, avgEnergy);
    }
    return true;
  }
  
  return false;
}

// Process audio input for voice commands
void processAudioInput() {
  if (!voiceDetectionEnabled) return;
  
  // Check for voice activity
  if (detectVoiceActivity()) {
    if (!isProcessingVoice) {
      // Start voice command processing
      isProcessingVoice = true;
      voiceCommandStart = millis();
      currentVoiceBuffer = "";
      Serial.println("🎤 Started voice command capture...");
      
      // Play a brief tone to indicate listening
      playTone(1000, 50);
    }
  }
  
  // If we're processing voice and timeout hasn't occurred
  if (isProcessingVoice) {
    unsigned long elapsed = millis() - voiceCommandStart;
    
    if (elapsed > voiceCommandWindow) {
      // Timeout - process what we have
      String command = processVoiceCommand();
      if (command != "") {
        handleVoiceCommand(command);
      } else {
        Serial.println("❌ Voice command timeout - no command recognized");
        playErrorSound();
      }
      isProcessingVoice = false;
    }
  }
}

// Process captured voice data into command (simplified pattern matching)
String processVoiceCommand() {
  // In a real implementation, this would:
  // 1. Use actual speech recognition (Google Speech API, Whisper, etc.)
  // 2. Convert audio to text
  // 3. Parse the text for commands
  
  // For this implementation, we'll use a simplified approach:
  // Detect patterns in the audio energy and duration to simulate basic command recognition
  
  unsigned long captureTime = millis() - voiceCommandStart;
  
  // Simple heuristic based on voice capture duration and recent activity
  if (captureTime > 500 && captureTime < 3000) {
    // Simulate command recognition based on timing patterns
    // In reality, you would analyze the actual audio content
    
    float recentActivity = 0;
    for (int i = 0; i < 10; i++) {
      recentActivity += voiceEnergyHistory[i];
    }
    
    // Use a simple pattern: longer utterances tend to be "turn on/off"
    // shorter ones might be "status"
    if (captureTime > 1000) {
      // Simulate alternating between turn on/off for demonstration
      static bool lastCommandWasOn = false;
      lastCommandWasOn = !lastCommandWasOn;
      return lastCommandWasOn ? "turn on" : "turn off";
    } else if (captureTime > 600) {
      return "status";
    }
  }
  
  return ""; // No command recognized
}

// Handle voice command
void handleVoiceCommand(String command) {
  Serial.println("🎤 Voice command recognized: " + command);
  
  // Convert to lowercase for matching
  command.toLowerCase();
  
  // Find matching command using enhanced pattern matching
  for (int i = 0; i < NUM_VOICE_COMMANDS; i++) {
    bool matched = false;
    
    // Check all patterns for this command
    for (int j = 0; j < voiceCommands[i].patternCount; j++) {
      if (command.indexOf(voiceCommands[i].patterns[j]) >= 0) {
        matched = true;
        break;
      }
    }
    
    if (matched) {
      String action = voiceCommands[i].action;
      String requestId = "voice_" + String(millis());
      
      Serial.println("✓ Executing voice command: " + action);
      
      if (action == "turn_on") {
        handleTurnOn(requestId, "voice");
        playConfirmationSound();
      } else if (action == "turn_off") {
        handleTurnOff(requestId, "voice");
        playConfirmationSound();
      } else if (action == "get_status") {
        handleGetStatus(requestId, "voice");
        playConfirmationSound();
      }
      
      // Publish voice command event to MQTT
      if (client.connected()) {
        DynamicJsonDocument doc(512);
        doc["deviceId"] = deviceId;
        doc["voiceCommand"] = command;
        doc["action"] = action;
        doc["timestamp"] = millis();
        doc["source"] = "voice";
        doc["requestId"] = requestId;
        
        String message;
        serializeJson(doc, message);
        client.publish(audio_topic, message.c_str());
        Serial.println("📡 Voice command published to MQTT");
      }
      
      return;
    }
  }
  
  // Command not recognized
  Serial.println("❌ Voice command not recognized: " + command);
  playErrorSound();
}

// MQTT message callback
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("📨 MQTT message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Parse JSON command
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("❌ Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }

  String command = doc["command"];
  String requestId = doc["requestId"];

  Serial.println("📱 Processing MQTT command: " + command);

  if (command == "turn_on") {
    handleTurnOn(requestId, "mqtt");
  } else if (command == "turn_off") {
    handleTurnOff(requestId, "mqtt");
  } else if (command == "get_status") {
    handleGetStatus(requestId, "mqtt");
  } else if (command == "enable_voice") {
    voiceDetectionEnabled = true;
    sendCommandResponse("enable_voice", requestId, true, "", "mqtt");
    playConfirmationSound();
    Serial.println("🎤 Voice detection enabled via MQTT");
  } else if (command == "disable_voice") {
    voiceDetectionEnabled = false;
    sendCommandResponse("disable_voice", requestId, true, "", "mqtt");
    playConfirmationSound();
    Serial.println("🔇 Voice detection disabled via MQTT");
  } else {
    Serial.println("❌ Unknown MQTT command: " + command);
    sendCommandResponse(command, requestId, false, "Unknown command", "mqtt");
    playErrorSound();
  }
}

// Reconnect to MQTT broker
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      
      // Subscribe to command topic
      client.subscribe(command_topic);
      Serial.print("Subscribed to: ");
      Serial.println(command_topic);
      
      // Send device registration message
      sendRegistration();
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Send device registration to MQTT
void sendRegistration() {
  DynamicJsonDocument doc(512);
  doc["deviceId"] = deviceId;
  doc["name"] = deviceName;
  doc["ip"] = WiFi.localIP().toString();
  doc["status"] = lightState;
  doc["timestamp"] = millis();
  doc["type"] = "registration";
  doc["capabilities"] = JsonArray();
  doc["capabilities"].add("relay_control");
  doc["capabilities"].add("voice_commands");
  doc["capabilities"].add("audio_feedback");
  
  String message;
  serializeJson(doc, message);
  
  client.publish(heartbeat_topic, message.c_str());
  Serial.println("Registration sent via MQTT");
}

// Send heartbeat via MQTT
void sendHeartbeat() {
  if (!client.connected()) {
    return;
  }
  
  DynamicJsonDocument doc(512);
  doc["deviceId"] = deviceId;
  doc["name"] = deviceName;
  doc["ip"] = WiFi.localIP().toString();
  doc["status"] = lightState;
  doc["timestamp"] = millis();
  doc["type"] = "heartbeat";
  doc["relay_pin"] = LIGHT_RELAY_PIN;
  doc["voice_enabled"] = voiceDetectionEnabled;
  doc["audio_pins"]["microphone"]["ws"] = I2S_WS;
  doc["audio_pins"]["microphone"]["sck"] = I2S_SCK;
  doc["audio_pins"]["microphone"]["sd"] = I2S_SD;
  doc["audio_pins"]["output"] = AUDIO_OUTPUT_PIN;
  
  String message;
  serializeJson(doc, message);
  
  if (client.publish(heartbeat_topic, message.c_str())) {
    Serial.println("Heartbeat sent via MQTT");
  } else {
    Serial.println("Failed to send heartbeat");
  }
}

// Send status via MQTT
void sendStatus(String requestId = "") {
  DynamicJsonDocument doc(512);
  doc["deviceId"] = deviceId;
  doc["status"] = lightState;
  doc["relay_pin"] = LIGHT_RELAY_PIN;
  doc["ip_address"] = WiFi.localIP().toString();
  doc["timestamp"] = millis();
  doc["type"] = "status";
  doc["voice_enabled"] = voiceDetectionEnabled;
  
  if (requestId != "") {
    doc["requestId"] = requestId;
  }
  
  String message;
  serializeJson(doc, message);
  
  const char* topic = (requestId != "") ? response_topic : status_topic;
  
  if (client.publish(topic, message.c_str())) {
    Serial.println("Status sent via MQTT to " + String(topic));
  } else {
    Serial.println("Failed to send status");
  }
}

// Send command response via MQTT
void sendCommandResponse(String command, String requestId, bool success, String error, String source) {
  if (!client.connected()) return;
  
  DynamicJsonDocument doc(512);
  doc["deviceId"] = deviceId;
  doc["command"] = command;
  doc["requestId"] = requestId;
  doc["success"] = success;
  doc["status"] = lightState;
  doc["timestamp"] = millis();
  doc["source"] = source;
  
  if (error != "") {
    doc["error"] = error;
  }
  
  String message;
  serializeJson(doc, message);
  
  if (client.publish(response_topic, message.c_str())) {
    Serial.println("📡 Command response sent via MQTT (" + source + ")");
  } else {
    Serial.println("❌ Failed to send command response");
  }
}

// Handle turn on command
void handleTurnOn(String requestId, String source) {
  Serial.println("💡 Command: Light turning ON (" + source + ")");
  lightState = "on";
  digitalWrite(LIGHT_RELAY_PIN, HIGH);  // HIGH turns relay ON
  
  if (saveState) {
    EEPROM.write(0, 1);
    EEPROM.commit();
  }
  
  sendCommandResponse("turn_on", requestId, true, "", source);
  sendStatus(""); // Also broadcast status update
  Serial.println("✅ Light turned ON via " + source);
}

// Handle turn off command
void handleTurnOff(String requestId, String source) {
  Serial.println("💡 Command: Light turning OFF (" + source + ")");
  lightState = "off";
  digitalWrite(LIGHT_RELAY_PIN, LOW); // LOW turns relay OFF
  
  if (saveState) {
    EEPROM.write(0, 0);
    EEPROM.commit();
  }
  
  sendCommandResponse("turn_off", requestId, true, "", source);
  sendStatus(""); // Also broadcast status update
  Serial.println("✅ Light turned OFF via " + source);
}

// Handle get status command
void handleGetStatus(String requestId, String source) {
  Serial.println("ℹ️ Command: Get status (" + source + ")");
  sendStatus(requestId);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("🚀 ESP32 Audio-Enabled Light Controller Starting...");
  
  // Initialize EEPROM
  if (saveState) {
    EEPROM.begin(eepromSize);
  }

  // Initialize the GPIO pin for the light relay as output
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  
  // Load saved state from EEPROM
  if (saveState) {
    lightState = EEPROM.read(0) == 1 ? "on" : "off";
  }

  // Set initial relay state
  digitalWrite(LIGHT_RELAY_PIN, lightState == "on" ? HIGH : LOW);
  
  Serial.println("💡 Light initial state: " + lightState);

  // Connect to Wi-Fi
  setup_wifi();
  
  // Setup OTA (Over-The-Air) programming
  if (WiFi.status() == WL_CONNECTED) {
    setupOTA();
  }
  
  // Setup Audio System
  Serial.println("🔊 Initializing audio system...");
  setupI2S();
  setupAudioOutput();
  
  // Initialize audio history arrays
  for (int i = 0; i < 10; i++) {
    voiceEnergyHistory[i] = 0;
  }
  for (int i = 0; i < 32; i++) {
    audioHistory[i] = 0;
  }
  
  // Setup MQTT
  Serial.println("📡 Initializing MQTT...");
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // Play startup sound
  delay(500);
  playStartupSound();
  
  Serial.println("✅ ESP32 Audio-Enabled Light Controller started successfully!");
  Serial.println("📋 Configuration:");
  Serial.println("  Device ID: " + String(deviceId));
  Serial.println("  Device Name: " + String(deviceName));
  Serial.println("  Relay Pin: GPIO " + String(LIGHT_RELAY_PIN));
  Serial.println("  Voice Detection: " + String(voiceDetectionEnabled ? "Enabled" : "Disabled"));
  Serial.println("  Sample Rate: " + String(SAMPLE_RATE) + " Hz");
  Serial.println("  Detection Threshold: " + String(DETECTION_THRESHOLD));
  Serial.println("📡 MQTT Topics:");
  Serial.println("  📥 Subscribe: " + String(command_topic));
  Serial.println("  📤 Status: " + String(status_topic));
  Serial.println("  💓 Heartbeat: " + String(heartbeat_topic));
  Serial.println("  📨 Responses: " + String(response_topic));
  Serial.println("  🎤 Audio Events: " + String(audio_topic));
  Serial.println("🎯 Ready for MQTT and voice commands!");
}

void loop() {
  unsigned long now = millis();
  
  // Handle OTA updates
  ArduinoOTA.handle();
  
  // Check WiFi connection periodically
  if (now - lastWiFiCheck > wifiCheckInterval) {
    checkWiFiConnection();
    lastWiFiCheck = now;
  }
  
  // Ensure MQTT connection
  if (!client.connected()) {
    if (now - lastReconnect > reconnectInterval) {
      lastReconnect = now;
      reconnect();
    }
  } else {
    client.loop();
    
    // Send periodic heartbeat
    if (now - lastHeartbeat > heartbeatInterval) {
      sendHeartbeat();
      lastHeartbeat = now;
    }
  }
  
  // Process audio input for voice commands (high frequency for responsiveness)
  if (now - lastAudioCheck > audioCheckInterval) {
    processAudioInput();
    lastAudioCheck = now;
  }
  
  // Small delay to prevent overwhelming the CPU but maintain responsiveness
  delay(20);
}


