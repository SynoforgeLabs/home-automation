const express = require('express');
const cors = require('cors');
const axios = require('axios');

const app = express();
const PORT = process.env.PORT || 3000;
const HOST = process.env.HOST || '0.0.0.0'; // Allow external connections for VPS

// Middleware
app.use(cors());
app.use(express.json());

// Store registered ESP32 devices
const devices = new Map();

console.log('ESP32 Bridge Server starting...');
console.log(`HTTP API will be available on port ${PORT}`);

// Device registration endpoint (replaces WebSocket registration)
app.post('/api/devices/register', (req, res) => {
  try {
    const { deviceId, name, ip, port } = req.body;
    
    if (!deviceId || !ip) {
      return res.status(400).json({ error: 'deviceId and ip are required' });
    }
    
    const deviceInfo = {
      id: deviceId,
      name: name || 'ESP32 Device',
      ip: ip,
      port: port || 80,
      lastSeen: new Date(),
      status: 'online'
    };
    
    devices.set(deviceId, deviceInfo);
    console.log(`Device registered: ${deviceId} at ${ip}:${port || 80}`);
    
    res.json({
      message: 'Device registered successfully',
      deviceId: deviceId
    });
  } catch (error) {
    console.error('Error registering device:', error);
    res.status(500).json({ error: 'Registration failed' });
  }
});

// Device heartbeat endpoint (replaces WebSocket heartbeat)
app.post('/api/devices/:deviceId/heartbeat', (req, res) => {
  const deviceId = req.params.deviceId;
  const device = devices.get(deviceId);
  
  if (device) {
    const previousStatus = device.status;
    device.lastSeen = new Date();
    device.status = 'online';
    
    if (previousStatus !== 'online') {
      console.log(`✓ Device ${deviceId} is back online (was ${previousStatus})`);
    } else {
      console.log(`♡ Heartbeat received from ${deviceId}`);
    }
    
    res.json({ 
      message: 'Heartbeat received',
      deviceStatus: device.status,
      timestamp: device.lastSeen
    });
  } else {
    console.log(`✗ Heartbeat from unknown device: ${deviceId}`);
    res.status(404).json({ error: 'Device not found' });
  }
});

// Clean up offline devices periodically
setInterval(() => {
  const now = new Date();
  const TIMEOUT = 60000; // 1 minute timeout
  
  for (const [deviceId, device] of devices.entries()) {
    const timeSinceLastSeen = now - device.lastSeen;
    if (timeSinceLastSeen > TIMEOUT && device.status === 'online') {
      console.log(`⚠ Device ${deviceId} marked as offline (no heartbeat for ${Math.round(timeSinceLastSeen/1000)}s)`);
      device.status = 'offline';
    }
  }
}, 30000); // Check every 30 seconds

// REST API Routes

// Get all devices
app.get('/api/devices', (req, res) => {
  const deviceList = Array.from(devices.values()).map(d => ({
    id: d.id,
    name: d.name,
    ip: d.ip,
    port: d.port,
    status: d.status,
    lastSeen: d.lastSeen
  }));
  
  res.json({ devices: deviceList });
});

// Get specific device status
app.get('/api/devices/:deviceId/status', async (req, res) => {
  const deviceId = req.params.deviceId;
  const device = devices.get(deviceId);
  
  if (!device) {
    return res.status(404).json({ error: 'Device not found' });
  }
  
  if (device.status !== 'online') {
    return res.status(503).json({ error: 'Device is offline' });
  }
  
  try {
    // Forward request to ESP32
    const response = await axios.get(`http://${device.ip}:${device.port}/status`, {
      timeout: 5000
    });
    
    res.json({
      device: {
        id: device.id,
        name: device.name,
        status: device.status
      },
      esp32: response.data
    });
  } catch (error) {
    console.error(`Error getting status from ${deviceId}:`, error.message);
    device.status = 'error';
    res.status(503).json({ 
      error: 'Failed to communicate with device',
      details: error.message 
    });
  }
});

// Control device - turn on
app.post('/api/devices/:deviceId/on', async (req, res) => {
  const deviceId = req.params.deviceId;
  const device = devices.get(deviceId);
  
  if (!device) {
    return res.status(404).json({ error: 'Device not found' });
  }
  
  if (device.status !== 'online') {
    return res.status(503).json({ error: 'Device is offline' });
  }
  
  try {
    // Forward request to ESP32
    const response = await axios.post(`http://${device.ip}:${device.port}/on`, {}, {
      timeout: 5000
    });
    
    res.json({
      device: {
        id: device.id,
        name: device.name,
        status: device.status
      },
      esp32: response.data
    });
  } catch (error) {
    console.error(`Error turning on ${deviceId}:`, error.message);
    device.status = 'error';
    res.status(503).json({ 
      error: 'Failed to communicate with device',
      details: error.message 
    });
  }
});

// Control device - turn off
app.post('/api/devices/:deviceId/off', async (req, res) => {
  const deviceId = req.params.deviceId;
  const device = devices.get(deviceId);
  
  if (!device) {
    return res.status(404).json({ error: 'Device not found' });
  }
  
  if (device.status !== 'online') {
    return res.status(503).json({ error: 'Device is offline' });
  }
  
  try {
    // Forward request to ESP32
    const response = await axios.post(`http://${device.ip}:${device.port}/off`, {}, {
      timeout: 5000
    });
    
    res.json({
      device: {
        id: device.id,
        name: device.name,
        status: device.status
      },
      esp32: response.data
    });
  } catch (error) {
    console.error(`Error turning off ${deviceId}:`, error.message);
    device.status = 'error';
    res.status(503).json({ 
      error: 'Failed to communicate with device',
      details: error.message 
    });
  }
});

// Health check endpoint
app.get('/health', (req, res) => {
  const deviceList = Array.from(devices.values()).map(d => ({
    id: d.id,
    status: d.status,
    lastSeen: d.lastSeen,
    timeSinceLastSeen: new Date() - d.lastSeen
  }));
  
  res.json({ 
    status: 'healthy', 
    timestamp: new Date(),
    connectedDevices: devices.size,
    devices: Array.from(devices.keys()),
    deviceDetails: deviceList
  });
});

// Default route
app.get('/', (req, res) => {
  res.json({
    message: 'ESP32 Bridge Server',
    version: '1.0.0',
    endpoints: [
      'POST /api/devices/register - Register a new device',
      'POST /api/devices/:deviceId/heartbeat - Send device heartbeat',
      'GET /api/devices - List all devices',
      'GET /api/devices/:deviceId/status - Get device status',
      'POST /api/devices/:deviceId/on - Turn device on',
      'POST /api/devices/:deviceId/off - Turn device off',
      'GET /health - Health check'
    ]
  });
});

app.listen(PORT, HOST, () => {
  console.log(`Bridge server running on ${HOST}:${PORT}`);
  console.log(`Visit http://${HOST}:${PORT} for API documentation`);
  console.log('Waiting for ESP32 devices to connect...');
});


