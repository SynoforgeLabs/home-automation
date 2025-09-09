#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <HTTPClient.h>

// Replace with your network credentials
const char* ssid = "SLT-Fiber-EYcM6-2.4G";  // Network SSID (name)
const char* password = "aqua1483";  // Network password

// Bridge server configuration (your laptop's local IP)
const char* bridgeServerHost = "192.168.1.1";  // Your laptop's local IP on same network  
const int bridgeServerPort = 3000;  // HTTP port
const char* deviceId = "esp32-light-controller";
const char* deviceName = "Living Room Light";

// Initialize the web server on port 80
WebServer server(80);

// Variables to store the current state of the light (ON/OFF)
String lightState = "off";

// Assign the light relay to a GPIO pin
const int LIGHT_RELAY_PIN = 4;  // GPIO 4 for the light relay

// Variable to enable or disable state saving
const bool saveState = true;

// EEPROM address to store the state
const int eepromSize = 1;

// Test bridge server connectivity
bool testBridgeConnection() {
  HTTPClient http;
  String url = "http://" + String(bridgeServerHost) + ":" + String(bridgeServerPort) + "/health";
  
  Serial.print("Testing bridge server connectivity: ");
  Serial.println(url);
  
  http.begin(url);
  http.setTimeout(5000); // 5 second timeout
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    Serial.println("✓ Bridge server is reachable");
    http.end();
    return true;
  } else if (httpResponseCode > 0) {
    Serial.print("✗ Bridge server responded with code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("✗ Failed to reach bridge server. Error: ");
    Serial.println(http.errorToString(httpResponseCode));
  }
  
  http.end();
  return false;
}

// Register this ESP32 with the bridge server via HTTP
void registerWithBridgeServer() {
  HTTPClient http;
  String url = "http://" + String(bridgeServerHost) + ":" + String(bridgeServerPort) + "/api/devices/register";
  
  Serial.print("Registering with bridge server at: ");
  Serial.println(url);
  Serial.print("Device IP: ");
  Serial.println(WiFi.localIP());
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000); // 15 second timeout for registration
  
  DynamicJsonDocument doc(300);
  doc["deviceId"] = deviceId;
  doc["name"] = deviceName;
  doc["ip"] = WiFi.localIP().toString();
  doc["port"] = 80;
  
  String message;
  serializeJson(doc, message);
  
  Serial.print("Registration payload: ");
  Serial.println(message);
  
  int httpResponseCode = http.POST(message);
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("✓ Successfully registered with bridge server");
    Serial.println("Response: " + response);
  } else if (httpResponseCode > 0) {
    Serial.print("✗ Registration failed. HTTP response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    if (response.length() > 0) {
      Serial.print("Error response: ");
      Serial.println(response);
    }
  } else {
    Serial.print("✗ Registration failed. Network error: ");
    Serial.println(http.errorToString(httpResponseCode));
  }
  
  http.end();
}

// Send heartbeat to bridge server via HTTP
void sendHeartbeat() {
  HTTPClient http;
  String url = "http://" + String(bridgeServerHost) + ":" + String(bridgeServerPort) + "/api/devices/" + String(deviceId) + "/heartbeat";
  
  Serial.print("Sending heartbeat to: ");
  Serial.println(url);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000); // 10 second timeout
  
  int httpResponseCode = http.POST("{}");
  
  if (httpResponseCode == 200) {
    Serial.println("✓ Heartbeat sent successfully");
  } else if (httpResponseCode > 0) {
    Serial.print("✗ Heartbeat failed. HTTP response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    if (response.length() > 0) {
      Serial.print("Response: ");
      Serial.println(response);
    }
  } else {
    Serial.print("✗ Heartbeat failed. Network error: ");
    Serial.println(http.errorToString(httpResponseCode));
  }
  
  http.end();
}

// Function to add CORS headers
void addCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// HTTP endpoint handlers
void handleStatus() {
  addCORSHeaders();
  
  DynamicJsonDocument doc(200);
  doc["status"] = lightState;
  doc["relay_pin"] = LIGHT_RELAY_PIN;
  doc["ip_address"] = WiFi.localIP().toString();
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
  Serial.println("HTTP GET /status - returned: " + response);
}

void handleTurnOn() {
  addCORSHeaders();
  
  Serial.println("HTTP POST /on - Light turning ON");
  lightState = "on";
  digitalWrite(LIGHT_RELAY_PIN, HIGH);  // HIGH turns relay ON (corrected logic)
  
  if (saveState) {
    EEPROM.write(0, 1);
    EEPROM.commit();
  }
  
  DynamicJsonDocument doc(150);
  doc["status"] = lightState;
  doc["message"] = "Light turned ON";
  doc["ip_address"] = WiFi.localIP().toString();
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
  Serial.println("HTTP Response: " + response);
}

void handleTurnOff() {
  addCORSHeaders();
  
  Serial.println("HTTP POST /off - Light turning OFF");
  lightState = "off";
  digitalWrite(LIGHT_RELAY_PIN, LOW); // LOW turns relay OFF (corrected logic)
  
  if (saveState) {
    EEPROM.write(0, 0);
    EEPROM.commit();
  }
  
  DynamicJsonDocument doc(150);
  doc["status"] = lightState;
  doc["message"] = "Light turned OFF";
  doc["ip_address"] = WiFi.localIP().toString();
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
  Serial.println("HTTP Response: " + response);
}

void handleOptions() {
  addCORSHeaders();
  server.send(200, "text/plain", "");
}

void handleNotFound() {
  addCORSHeaders();
  
  DynamicJsonDocument doc(200);
  doc["error"] = "Endpoint not found";
  doc["available_endpoints"] = "[GET /status, POST /on, POST /off]";
  
  String response;
  serializeJson(doc, response);
  
  server.send(404, "application/json", response);
}


void setup() {
  Serial.begin(115200);
  
  // Initialize EEPROM
  if (saveState) {
    EEPROM.begin(eepromSize);
  }

  // Initialize the GPIO pin for the light relay as output and set to LOW (OFF state)
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  
  // Load saved state from EEPROM
  if (saveState) {
    lightState = EEPROM.read(0) == 1 ? "on" : "off";
  }

  // Set initial relay state (corrected logic: HIGH=ON, LOW=OFF)
  digitalWrite(LIGHT_RELAY_PIN, lightState == "on" ? HIGH : LOW);
  
  Serial.print("Light initial state: ");
  Serial.println(lightState);

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup HTTP server routes
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/on", HTTP_POST, handleTurnOn);
  server.on("/off", HTTP_POST, handleTurnOff);
  server.on("/", HTTP_OPTIONS, handleOptions);
  server.on("/status", HTTP_OPTIONS, handleOptions);
  server.on("/on", HTTP_OPTIONS, handleOptions);
  server.on("/off", HTTP_OPTIONS, handleOptions);
  server.onNotFound(handleNotFound);
  
  // Start the HTTP server
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Available endpoints:");
  Serial.println("  GET  /status - Get current light status");
  Serial.println("  POST /on     - Turn light ON");
  Serial.println("  POST /off    - Turn light OFF");
  
  // Test bridge server connectivity and register
  delay(2000); // Wait a bit for WiFi to fully stabilize
  
  Serial.println("=== BRIDGE SERVER CONNECTION TEST ===");
  if (testBridgeConnection()) {
    Serial.println("Bridge server is reachable, proceeding with registration...");
    registerWithBridgeServer();
  } else {
    Serial.println("WARNING: Bridge server is not reachable!");
    Serial.println("Please check:");
    Serial.println("1. Bridge server is running on " + String(bridgeServerHost) + ":" + String(bridgeServerPort));
    Serial.println("2. ESP32 and laptop are on the same network");
    Serial.println("3. Firewall settings allow connections");
    Serial.println("Will retry connection in main loop...");
  }
  
  Serial.print("Target bridge server: ");
  Serial.print(bridgeServerHost);
  Serial.print(":");
  Serial.println(bridgeServerPort);
  Serial.println("=== ESP32 READY ===");
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Attempting to reconnect...");
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("WiFi reconnected!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      // Re-register with bridge server after reconnection
      registerWithBridgeServer();
    } else {
      Serial.println("");
      Serial.println("Failed to reconnect to WiFi");
    }
  }
  
  // Handle HTTP server requests
  server.handleClient();
  
  // Send heartbeat every 30 seconds (only if WiFi is connected)
  static unsigned long lastHeartbeat = 0;
  static unsigned long lastConnectionTest = 0;
  static bool bridgeServerReachable = true;
  
  if (WiFi.status() == WL_CONNECTED && (millis() - lastHeartbeat > 30000)) {
    sendHeartbeat();
    lastHeartbeat = millis();
  }
  
  // Test bridge server connectivity every 2 minutes if previous heartbeats failed
  if (WiFi.status() == WL_CONNECTED && !bridgeServerReachable && (millis() - lastConnectionTest > 120000)) {
    Serial.println("Retesting bridge server connectivity...");
    bridgeServerReachable = testBridgeConnection();
    if (bridgeServerReachable) {
      Serial.println("Bridge server is back online! Re-registering...");
      registerWithBridgeServer();
    }
    lastConnectionTest = millis();
  }
  
  // Small delay to prevent overwhelming the CPU
  delay(100);
}