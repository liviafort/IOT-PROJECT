import express from 'express';
import { createServer } from 'http';
import { Server as SocketIOServer } from 'socket.io';
import cors from 'cors';
import mqtt from 'mqtt';
import { Pool } from 'pg';

// ============================================================================
// CONFIGURAÇÃO
// ============================================================================
const CONFIG = {
  PORT: 3000,
  MQTT_BROKER: 'mqtt://mosquitto:1883',
  MQTT_TOPIC: 'iot/sensor/dados',
  DB_HOST: 'postgres',
  DB_PORT: 5432,
  DB_NAME: 'iot_data',
  DB_USER: 'postgres',
  DB_PASSWORD: 'postgres'
};

const app = express();
const httpServer = createServer(app);
const io = new SocketIOServer(httpServer, {
  cors: { origin: '*', methods: ['GET', 'POST'] }
});

app.use(cors());
app.use(express.json());

// PostgreSQL
const pool = new Pool({
  host: CONFIG.DB_HOST,
  port: CONFIG.DB_PORT,
  database: CONFIG.DB_NAME,
  user: CONFIG.DB_USER,
  password: CONFIG.DB_PASSWORD
});

// Criar tabela
const initDB = async () => {
  const query = `
    CREATE TABLE IF NOT EXISTS sensor_data (
      id SERIAL PRIMARY KEY,
      device VARCHAR(100) NOT NULL,
      timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
      temperatura DECIMAL(5,2),
      umidade DECIMAL(5,2),
      luminosidade INTEGER,
      rssi INTEGER,
      uptime INTEGER,
      raw_data JSONB
    );
    CREATE INDEX IF NOT EXISTS idx_device ON sensor_data(device);
    CREATE INDEX IF NOT EXISTS idx_timestamp ON sensor_data(timestamp DESC);
  `;
  try {
    await pool.query(query);
    console.log('✅ Database initialized');
  } catch (error) {
    console.error('❌ Database error:', error);
  }
};

initDB();

// MQTT Client - Escuta broker e salva no banco
const mqttClient = mqtt.connect(CONFIG.MQTT_BROKER);

mqttClient.on('connect', () => {
  console.log('✅ Connected to MQTT Broker');
  mqttClient.subscribe(CONFIG.MQTT_TOPIC, (err) => {
    if (!err) console.log(`📡 Subscribed: ${CONFIG.MQTT_TOPIC}`);
  });
});

mqttClient.on('message', async (topic, message) => {
  try {
    const data = JSON.parse(message.toString());
    console.log('📨 MQTT:', data);

    // Salvar no banco
    const result = await pool.query(
      `INSERT INTO sensor_data (device, temperatura, umidade, luminosidade, rssi, uptime, raw_data)
       VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING *`,
      [
        data.device || 'unknown',
        data.temperatura,
        data.umidade,
        data.luminosidade,
        data.rssi,
        data.uptime,
        data
      ]
    );

    const saved = result.rows[0];
    console.log('💾 Saved:', saved.id);

    // Emitir via WebSocket
    io.emit('sensor-data', saved);
  } catch (error) {
    console.error('❌ Error:', error);
  }
});

// ============================================================================
// ROTAS API
// ============================================================================

app.get('/health', (req, res) => {
  res.json({ status: 'ok', mqtt: mqttClient.connected });
});

// GET all data
app.get('/api/sensor-data', async (req, res) => {
  try {
    const limit = parseInt(req.query.limit as string) || 100;
    const device = req.query.device as string;

    let query = 'SELECT * FROM sensor_data';
    let params: any[] = [];

    if (device) {
      query += ' WHERE device = $1';
      params.push(device);
    }

    query += ' ORDER BY timestamp DESC LIMIT $' + (params.length + 1);
    params.push(limit);

    const result = await pool.query(query, params);
    res.json({ success: true, count: result.rows.length, data: result.rows });
  } catch (error) {
    res.status(500).json({ success: false, error: 'Internal error' });
  }
});

// GET latest by device
app.get('/api/sensor-data/latest/:device', async (req, res) => {
  try {
    const result = await pool.query(
      'SELECT * FROM sensor_data WHERE device = $1 ORDER BY timestamp DESC LIMIT 1',
      [req.params.device]
    );
    if (result.rows.length === 0) {
      return res.status(404).json({ success: false, error: 'Device not found' });
    }
    res.json({ success: true, data: result.rows[0] });
  } catch (error) {
    res.status(500).json({ success: false, error: 'Internal error' });
  }
});

// GET devices list
app.get('/api/devices', async (req, res) => {
  try {
    const result = await pool.query(`
      SELECT device, COUNT(*) as total_readings, MAX(timestamp) as last_reading
      FROM sensor_data GROUP BY device ORDER BY last_reading DESC
    `);
    res.json({ success: true, data: result.rows });
  } catch (error) {
    res.status(500).json({ success: false, error: 'Internal error' });
  }
});

// WebSocket
io.on('connection', (socket) => {
  console.log('🔌 Client connected:', socket.id);
  socket.on('disconnect', () => console.log('🔌 Disconnected:', socket.id));
});

// Start server
httpServer.listen(CONFIG.PORT, () => {
  console.log('\n================================');
  console.log('🚀 IoT Backend Server');
  console.log('================================');
  console.log(`HTTP: http://localhost:${CONFIG.PORT}`);
  console.log(`WebSocket: ws://localhost:${CONFIG.PORT}`);
  console.log(`MQTT: ${CONFIG.MQTT_BROKER}`);
  console.log('================================\n');
});

// Graceful shutdown
process.on('SIGINT', async () => {
  console.log('\n🛑 Shutting down...');
  mqttClient.end();
  await pool.end();
  httpServer.close();
  process.exit(0);
});
