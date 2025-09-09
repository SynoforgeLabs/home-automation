# VPS Setup Guide for ESP32 Bridge Server

## 1. Connect to Your VPS
```bash
ssh your-username@your-vps-ip
# or if using Tailscale:
ssh your-username@your-tailscale-ip
```

## 2. Install Dependencies
```bash
# Update system
sudo pacman -Syu

# Install Node.js and npm
sudo pacman -S nodejs npm

# Install PM2 for process management (optional but recommended)
sudo npm install -g pm2
```

## 3. Create Bridge Server Directory
```bash
# Create project directory
mkdir -p ~/esp32-bridge
cd ~/esp32-bridge

# Create package.json
cat > package.json << 'EOF'
{
  "name": "esp32-bridge-server",
  "version": "1.0.0",
  "description": "HTTP bridge server for ESP32 remote access",
  "main": "server.js",
  "scripts": {
    "start": "node server.js",
    "dev": "nodemon server.js"
  },
  "dependencies": {
    "express": "^4.18.2",
    "cors": "^2.8.5",
    "axios": "^1.6.0",
    "ws": "^8.14.0"
  },
  "devDependencies": {
    "nodemon": "^3.0.1"
  },
  "keywords": ["esp32", "iot", "bridge", "http"],
  "author": "",
  "license": "MIT"
}
EOF
```

## 4. Install Node.js Dependencies
```bash
npm install
```

## 5. Create the Bridge Server
Create the server.js file:
```bash
cat > server.js << 'EOF'
const express = require('express');
const cors = require('cors');
const axios = require('axios');
const WebSocket = require('ws');

const app = express();
const PORT = process.env.PORT || 3000;

// Middleware
app.use(cors());
app.use(express.json());

// Store registered ESP32 devices
const devices = new Map();

// WebSocket server for real-time device management
const wss = new WebSocket.Server({ port: 8080 });

console.log('ESP32 Bridge Server starting...');
console.log(`HTTP API will be available on port ${PORT}`);
console.log('WebSocket server listening on port 8080');

// Device registration and management
wss.on('connection', (ws, req) => {
  console.log('New WebSocket connection from:', req.socket.remoteAddress);
  
  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      console.log('Received message:', data);
      
      if (data.type === 'register') {
        // ESP32 registering itself
        const deviceId = data.deviceId || 'esp32-light-controller';
        const deviceInfo = {
          id: deviceId,
          name: data.name || 'ESP32 Light Controller',
          ip: data.ip,
          port: data.port || 80,
          lastSeen: new Date(),
          ws: ws,
          status: 'online'
        };
        
        devices.set(deviceId, deviceInfo);
        console.log(`Device registered: ${deviceId} at ${data.ip}:${data.port}`);
        
        // Send confirmation back to device
        ws.send(JSON.stringify({
          type: 'registered',
          deviceId: deviceId,
          message: 'Device registered successfully'
        }));
        
        // Broadcast device list update to all clients
        broadcastDeviceList();
        
      } else if (data.type === 'heartbeat') {
        // Update last seen time
        const device = devices.get(data.deviceId);
        if (device) {
          device.lastSeen = new Date();
          device.status = 'online';
        }
      }
    } catch (error) {
      console.error('Error processing WebSocket message:', error);
    }
  });
  
  ws.on('close', () => {
    // Mark device as offline when WebSocket closes
    for (const [deviceId, device] of devices.entries()) {
      if (device.ws === ws) {
        device.status = 'offline';
        console.log(`Device ${deviceId} disconnected`);
        broadcastDeviceList();
        break;
      }
    }
  });
  
  // Send current device list to new client
  ws.send(JSON.stringify({
    type: 'device_list',
    devices: Array.from(devices.values()).map(d => ({
      id: d.id,
      name: d.name,
      ip: d.ip,
      port: d.port,
      status: d.status,
      lastSeen: d.lastSeen
    }))
  }));
});

function broadcastDeviceList() {
  const deviceList = Array.from(devices.values()).map(d => ({
    id: d.id,
    name: d.name,
    ip: d.ip,
    port: d.port,
    status: d.status,
    lastSeen: d.lastSeen
  }));
  
  const message = JSON.stringify({
    type: 'device_list',
    devices: deviceList
  });
  
  wss.clients.forEach(client => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(message);
    }
  });
}

// Clean up offline devices periodically
setInterval(() => {
  const now = new Date();
  const TIMEOUT = 60000; // 1 minute timeout
  
  for (const [deviceId, device] of devices.entries()) {
    if (now - device.lastSeen > TIMEOUT) {
      device.status = 'offline';
    }
  }
  
  broadcastDeviceList();
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
  res.json({ 
    status: 'healthy', 
    timestamp: new Date(),
    connectedDevices: devices.size,
    devices: Array.from(devices.keys())
  });
});

// Default route
app.get('/', (req, res) => {
  res.json({
    message: 'ESP32 Bridge Server',
    version: '1.0.0',
    endpoints: [
      'GET /api/devices - List all devices',
      'GET /api/devices/:deviceId/status - Get device status',
      'POST /api/devices/:deviceId/on - Turn device on',
      'POST /api/devices/:deviceId/off - Turn device off',
      'GET /health - Health check'
    ],
    websocket: 'ws://localhost:8080 - Device registration and real-time updates'
  });
});

app.listen(PORT, () => {
  console.log(`Bridge server running on port ${PORT}`);
  console.log(`Visit http://localhost:${PORT} for API documentation`);
  console.log('Waiting for ESP32 devices to connect...');
});
EOF
```

## 6. Test the Server
```bash
# Test run first
node server.js
```

You should see:
```
ESP32 Bridge Server starting...
HTTP API will be available on port 3000
WebSocket server listening on port 8080
Bridge server running on port 3000
Visit http://localhost:3000 for API documentation
Waiting for ESP32 devices to connect...
```

Press `Ctrl+C` to stop the test.

## 7. Run with PM2 (Production Mode)
```bash
# Start with PM2
pm2 start server.js --name "esp32-bridge"

# Save PM2 configuration
pm2 save

# Setup PM2 to start on boot
pm2 startup
# Follow the instructions shown (will give you a sudo command to run)

# Check status
pm2 status
pm2 logs esp32-bridge
```

## 8. Open Firewall Ports
```bash
# If using UFW firewall
sudo ufw allow 3000/tcp
sudo ufw allow 8080/tcp

# If using iptables directly
sudo iptables -A INPUT -p tcp --dport 3000 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 8080 -j ACCEPT
```

## 9. Get Your Server IP
```bash
# Get VPS public IP
curl ifconfig.me

# Get Tailscale IP (if using Tailscale)
tailscale ip -4
```

## 10. Test the API
```bash
# Test health check
curl http://localhost:3000/health

# Test from outside (replace YOUR_VPS_IP with actual IP)
curl http://YOUR_VPS_IP:3000/health
```

## 11. Update Your Code
After the server is running, update these files:

**ESP32 Code** (`mqtt_relay_controller.ino` line 13):
```cpp
const char* bridgeServerHost = "YOUR_VPS_IP_HERE";  // Replace with your actual VPS IP
```

**Mobile App** (`App.tsx` line 41):
```typescript
const bridgeUrl = 'http://YOUR_VPS_IP_HERE:3000'; // Replace with your actual VPS IP
```

## 12. Monitor the Server
```bash
# View logs
pm2 logs esp32-bridge

# Restart if needed
pm2 restart esp32-bridge

# Stop
pm2 stop esp32-bridge
```

## Tailscale Security (Optional)
If you want to use Tailscale for secure access:

1. **Install Tailscale on VPS:**
```bash
curl -fsSL https://tailscale.com/install.sh | sh
sudo tailscale up
```

2. **Get Tailscale IP:**
```bash
tailscale ip -4
```

3. **Use Tailscale IP in your code instead of public IP**

This way, only devices on your Tailscale network can access the bridge server.


