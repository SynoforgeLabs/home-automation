const express = require('express');
const cors = require('cors');
const mqtt = require('mqtt');

const app = express();
const PORT = process.env.PORT || 3005;
const HOST = process.env.HOST || '0.0.0.0'; // Allow external connections for VPS

// MQTT Configuration
const MQTT_BROKER = process.env.MQTT_BROKER || 'mqtt://broker.hivemq.com:1883';
const MQTT_TOPICS = {
  DEVICE_HEARTBEAT: 'devices/+/heartbeat',
  DEVICE_STATUS: 'devices/+/status', 
  DEVICE_RESPONSES: 'devices/+/responses',
  DEVICE_AUDIO: 'devices/+/audio',
  DEVICE_COMMANDS: 'devices/esp32-light-controller/commands'
};

// Middleware
app.use(cors());
app.use(express.json());

// Store registered ESP32 devices and pending requests
const devices = new Map();
const pendingRequests = new Map();

console.log('ESP32 MQTT Bridge Server starting...');
console.log(`HTTP API will be available on port ${PORT}`);
console.log(`Connecting to MQTT broker: ${MQTT_BROKER}`);

// Initialize MQTT client
const mqttClient = mqtt.connect(MQTT_BROKER, {
  clientId: `bridge-server-${Date.now()}`,
  keepalive: 60,
  reconnectPeriod: 1000,
  clean: true
});

mqttClient.on('connect', () => {
  console.log('✓ Connected to MQTT broker');
  
  // Subscribe to all device topics
  Object.values(MQTT_TOPICS).forEach(topic => {
    if (topic.includes('+')) { // Wildcard topics
      mqttClient.subscribe(topic, (err) => {
        if (err) {
          console.error(`Failed to subscribe to ${topic}:`, err);
        } else {
          console.log(`Subscribed to: ${topic}`);
        }
      });
    }
  });
});

mqttClient.on('error', (error) => {
  console.error('MQTT connection error:', error);
});

mqttClient.on('reconnect', () => {
  console.log('Reconnecting to MQTT broker...');
});

mqttClient.on('message', (topic, message) => {
  try {
    const messageStr = message.toString();
    
    // Skip non-JSON messages (common on public MQTT brokers)
    if (!messageStr.startsWith('{') && !messageStr.startsWith('[')) {
      console.log(`Skipping non-JSON message [${topic}]: ${messageStr}`);
      return;
    }
    
    const data = JSON.parse(messageStr);
    const topicParts = topic.split('/');
    const deviceId = topicParts[1];
    const messageType = topicParts[2];
    
    // Only log messages from our expected device or show count for others
    if (deviceId === 'esp32-light-controller') {
      console.log(`📡 MQTT Message [${topic}]:`, data);
    } else {
      console.log(`🌍 Other device message [${topic}]: ${Object.keys(data).join(', ')}`);
    }
    
    switch (messageType) {
      case 'heartbeat':
        handleHeartbeat(deviceId, data);
        break;
      case 'status':
        handleStatusUpdate(deviceId, data);
        break;
      case 'responses':
        handleCommandResponse(deviceId, data);
        break;
      case 'audio':
        handleAudioEvent(deviceId, data);
        break;
      default:
        console.log(`Unknown message type: ${messageType}`);
    }
  } catch (error) {
    console.error('Error parsing MQTT message:', error);
  }
});

// Handle device heartbeat/registration
function handleHeartbeat(deviceId, data) {
  const now = new Date();
  const existingDevice = devices.get(deviceId);
  
  const deviceInfo = {
    id: deviceId,
    name: data.name || deviceId,
    ip: data.ip,
    status: data.status || 'on',
    lastSeen: now,
    online: true,
    relayPin: data.relay_pin,
    // Enhanced audio/voice capabilities tracking
    voiceEnabled: data.voice_enabled || false,
    audioPins: data.audio_pins || null,
    capabilities: data.capabilities || []
  };
  
  devices.set(deviceId, deviceInfo);
  
  if (data.type === 'registration') {
    console.log(`📝 Device registered: ${deviceId} (${data.name}) at ${data.ip}`);
    if (data.capabilities) {
      console.log(`   Capabilities: ${data.capabilities.join(', ')}`);
    }
    if (data.voice_enabled) {
      console.log(`   🎤 Voice commands: ${data.voice_enabled ? 'Enabled' : 'Disabled'}`);
    }
  } else if (!existingDevice || !existingDevice.online) {
    console.log(`✓ Device ${deviceId} is online`);
  } else {
    console.log(`♡ Heartbeat from ${deviceId} (voice: ${data.voice_enabled ? 'on' : 'off'})`);
  }
}

// Handle audio/voice events from devices
function handleAudioEvent(deviceId, data) {
  const device = devices.get(deviceId);
  if (device) {
    device.lastSeen = new Date();
    console.log(`🎤 Voice command from ${deviceId}: "${data.voiceCommand}" → ${data.action} (${data.source || 'voice'})`);
    
    // You could store voice command history, analytics, etc. here
    // For now, just log the event
  }
}

// Handle status updates from devices
function handleStatusUpdate(deviceId, data) {
  const device = devices.get(deviceId);
  if (device) {
    device.status = data.status;
    device.lastSeen = new Date();
    console.log(`📊 Status update from ${deviceId}: ${data.status}`);
  }
}

// Handle command responses from devices
function handleCommandResponse(deviceId, data) {
  const requestId = data.requestId;
  if (requestId && pendingRequests.has(requestId)) {
    const { res, timeout } = pendingRequests.get(requestId);
    clearTimeout(timeout);
    pendingRequests.delete(requestId);
    
    // Check if this is an error response (has error field) or successful response
    const isSuccess = !data.error && (data.success !== false);
    
    if (isSuccess) {
      // Build response based on message type
      const response = {
        device: {
          id: deviceId,
          status: data.status
        },
        timestamp: data.timestamp,
        success: true
      };
      
      // Add command field if present (for control commands)
      if (data.command) {
        response.command = data.command;
      }
      
      // For status requests, include ESP32 data
      if (data.type === 'status') {
        response.esp32 = {
          status: data.status,
          relay_pin: data.relay_pin,
          ip_address: data.ip_address
        };
      }
      
      res.json(response);
    } else {
      res.status(500).json({
        error: 'Command failed',
        details: data.error || 'Unknown error'
      });
    }
    
    console.log(`📤 Command response sent for request ${requestId}: ${isSuccess ? 'success' : 'failed'}`);
  } else {
    console.log(`⚠ Received response for unknown request: ${requestId}`);
  }
}

// Send MQTT command to device
function sendMQTTCommand(deviceId, command, res, timeout = 10000) {
  const requestId = `req_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  
  const commandMessage = {
    command: command,
    requestId: requestId,
    timestamp: Date.now()
  };
  
  const topic = `devices/${deviceId}/commands`;
  
  // Store pending request
  const timeoutId = setTimeout(() => {
    if (pendingRequests.has(requestId)) {
      pendingRequests.delete(requestId);
      res.status(504).json({ 
        error: 'Request timeout',
        details: `Device did not respond within ${timeout/1000} seconds`
      });
    }
  }, timeout);
  
  pendingRequests.set(requestId, { res, timeout: timeoutId });
  
  // Publish command
  mqttClient.publish(topic, JSON.stringify(commandMessage), (err) => {
    if (err) {
      clearTimeout(timeoutId);
      pendingRequests.delete(requestId);
      console.error(`Failed to send command to ${deviceId}:`, err);
      res.status(500).json({ 
        error: 'Failed to send command',
        details: err.message 
      });
    } else {
      console.log(`📤 Command sent to ${deviceId}: ${command} (Request ID: ${requestId})`);
    }
  });
}

// Clean up offline devices periodically
setInterval(() => {
  const now = new Date();
  const TIMEOUT = 45000; // 45 second timeout (3x heartbeat interval)
  
  for (const [deviceId, device] of devices.entries()) {
    const timeSinceLastSeen = now - device.lastSeen;
    if (timeSinceLastSeen > TIMEOUT && device.online) {
      console.log(`⚠ Device ${deviceId} marked as offline (no heartbeat for ${Math.round(timeSinceLastSeen/1000)}s)`);
      device.online = false;
    }
  }
}, 30000); // Check every 30 seconds

// Clean up expired pending requests
setInterval(() => {
  const now = Date.now();
  for (const [requestId, { timestamp }] of pendingRequests.entries()) {
    if (timestamp && (now - timestamp) > 30000) { // 30 second cleanup
      pendingRequests.delete(requestId);
      console.log(`🧹 Cleaned up expired request: ${requestId}`);
    }
  }
}, 60000); // Clean up every minute

// REST API Routes

// Get all devices
app.get('/api/devices', (req, res) => {
  const deviceList = Array.from(devices.values()).map(d => ({
    id: d.id,
    name: d.name,
    ip: d.ip,
    status: d.status,
    online: d.online,
    lastSeen: d.lastSeen,
    voiceEnabled: d.voiceEnabled,
    capabilities: d.capabilities,
    audioPins: d.audioPins
  }));
  
  res.json({ devices: deviceList });
});

// Get specific device status
app.get('/api/devices/:deviceId/status', (req, res) => {
  const deviceId = req.params.deviceId;
  const device = devices.get(deviceId);
  
  if (!device) {
    return res.status(404).json({ error: 'Device not found' });
  }
  
  if (!device.online) {
    return res.status(503).json({ error: 'Device is offline' });
  }
  
  // Send get_status command via MQTT and wait for response
  sendMQTTCommand(deviceId, 'get_status', res);
});

// Control device - turn on
app.post('/api/devices/:deviceId/on', (req, res) => {
  const deviceId = req.params.deviceId;
  const device = devices.get(deviceId);
  
  if (!device) {
    return res.status(404).json({ error: 'Device not found' });
  }
  
  if (!device.online) {
    return res.status(503).json({ error: 'Device is offline' });
  }
  
  // Send turn_on command via MQTT and wait for response
  sendMQTTCommand(deviceId, 'turn_on', res);
});

// Control device - turn off
app.post('/api/devices/:deviceId/off', (req, res) => {
  const deviceId = req.params.deviceId;
  const device = devices.get(deviceId);
  
  if (!device) {
    return res.status(404).json({ error: 'Device not found' });
  }
  
  if (!device.online) {
    return res.status(503).json({ error: 'Device is offline' });
  }
  
  // Send turn_off command via MQTT and wait for response
  sendMQTTCommand(deviceId, 'turn_off', res);
});

// Voice control - enable voice detection
app.post('/api/devices/:deviceId/voice/enable', (req, res) => {
  const deviceId = req.params.deviceId;
  const device = devices.get(deviceId);
  
  if (!device) {
    return res.status(404).json({ error: 'Device not found' });
  }
  
  if (!device.online) {
    return res.status(503).json({ error: 'Device is offline' });
  }
  
  // Send enable_voice command via MQTT and wait for response
  sendMQTTCommand(deviceId, 'enable_voice', res);
});

// Voice control - disable voice detection
app.post('/api/devices/:deviceId/voice/disable', (req, res) => {
  const deviceId = req.params.deviceId;
  const device = devices.get(deviceId);
  
  if (!device) {
    return res.status(404).json({ error: 'Device not found' });
  }
  
  if (!device.online) {
    return res.status(503).json({ error: 'Device is offline' });
  }
  
  // Send disable_voice command via MQTT and wait for response
  sendMQTTCommand(deviceId, 'disable_voice', res);
});

// Health check endpoint
app.get('/health', (req, res) => {
  const deviceList = Array.from(devices.values()).map(d => ({
    id: d.id,
    status: d.status,
    online: d.online,
    lastSeen: d.lastSeen,
    timeSinceLastSeen: new Date() - d.lastSeen
  }));
  
  res.json({ 
    status: 'healthy', 
    timestamp: new Date(),
    mqttConnected: mqttClient.connected,
    connectedDevices: Array.from(devices.values()).filter(d => d.online).length,
    totalDevices: devices.size,
    devices: Array.from(devices.keys()),
    deviceDetails: deviceList
  });
});

// Default route
app.get('/', (req, res) => {
  res.json({
    message: 'ESP32 MQTT Bridge Server',
    version: '2.0.0',
    mqttBroker: MQTT_BROKER,
    mqttConnected: mqttClient.connected,
    endpoints: [
      'GET /api/devices - List all devices',
      'GET /api/devices/:deviceId/status - Get device status via MQTT',
      'POST /api/devices/:deviceId/on - Turn device on via MQTT',
      'POST /api/devices/:deviceId/off - Turn device off via MQTT',
      'POST /api/devices/:deviceId/voice/enable - Enable voice detection',
      'POST /api/devices/:deviceId/voice/disable - Disable voice detection',
      'GET /health - Health check'
    ],
    mqttTopics: MQTT_TOPICS
  });
});

app.listen(PORT, HOST, () => {
  console.log(`Bridge server running on ${HOST}:${PORT}`);
  console.log(`Visit http://${HOST}:${PORT} for API documentation`);
  console.log('Waiting for ESP32 devices to connect via MQTT...');
});