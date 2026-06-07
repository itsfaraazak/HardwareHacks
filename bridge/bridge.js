#!/usr/bin/env node
/**
 * RhythmRide — Laptop bridge
 *
 * Connects Arduino Uno (USB Serial) to the Cloudflare Worker relay.
 *
 *   Uno → Serial → bridge → Worker (wss) → Judge's Phone
 *   Phone → Worker (wss) → bridge → Serial → Uno
 *
 * Run:
 *   cd bridge && npm install && npm start
 *
 * Config (bridge/config.json):
 *   { "workerUrl": "wss://rhythmride-relay.YOUR_SUBDOMAIN.workers.dev/ws", "baudRate": 115200 }
 *
 * Or set env var:  WORKER_URL=wss://...  node bridge.js
 */

'use strict';

const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const WebSocket = require('ws');
const fs = require('fs');
const path = require('path');

// ── Config ────────────────────────────────────────────────────────────────────
let config = { workerUrl: '', baudRate: 115200 };
const cfgPath = path.join(__dirname, 'config.json');
if (fs.existsSync(cfgPath)) {
  try { Object.assign(config, JSON.parse(fs.readFileSync(cfgPath, 'utf8'))); }
  catch (e) { console.error('[CONFIG] Failed to parse config.json:', e.message); }
}
if (process.env.WORKER_URL) config.workerUrl = process.env.WORKER_URL;
if (!config.workerUrl || config.workerUrl.includes('YOUR_SUBDOMAIN')) {
  console.error('[CONFIG] ERROR: workerUrl not set in bridge/config.json or WORKER_URL env var.');
  console.error('         Edit bridge/config.json and fill in your Cloudflare Worker URL.');
  process.exit(1);
}

const BAUD = config.baudRate || 115200;
const WS_URL = config.workerUrl.replace(/\?.*$/, '') + '?role=bridge';

// ── Serial auto-detect ────────────────────────────────────────────────────────
async function findSerialPort() {
  if (process.env.SERIAL_PATH) return process.env.SERIAL_PATH;
  if (config.serialPath) return config.serialPath;
  const ports = await SerialPort.list();
  const platform = process.platform;

  // Preference order: Uno/CH340/CP210x identifiers
  const patterns = platform === 'win32'
    ? [/^COM\d+/]
    : platform === 'darwin'
      ? [/cu\.usbmodem/, /cu\.usbserial/, /cu\.SLAB_USB/, /cu\.wchusbserial/]
      : [/ttyACM/, /ttyUSB/];

  for (const pattern of patterns) {
    const match = ports.find(p => pattern.test(p.path));
    if (match) return match.path;
  }

  // Fallback: return first port
  if (ports.length) return ports[0].path;
  return null;
}

// ── State ─────────────────────────────────────────────────────────────────────
let serial = null;
let serialWriter = null;
let ws = null;
let wsBackoff = 500; // ms, doubles on each failure, capped at 5000ms
let serialReady = false;
let wsReady = false;

// ── WebSocket connection ──────────────────────────────────────────────────────
function connectWs() {
  if (ws) { try { ws.terminate(); } catch (_) {} }

  console.log(`[WS] Connecting to ${WS_URL} …`);
  ws = new WebSocket(WS_URL);

  ws.on('open', () => {
    wsBackoff = 500;
    wsReady = true;
    console.log('[WS] Connected to Cloudflare relay');
    printStatus();
  });

  ws.on('message', (data) => {
    const text = data.toString();
    console.log('[←phone]', text);
    if (serialWriter) {
      serialWriter.write(text + '\n', (err) => {
        if (err) console.error('[SERIAL] Write error:', err.message);
      });
    }
  });

  ws.on('close', (code, reason) => {
    wsReady = false;
    console.log(`[WS] Disconnected (${code}). Reconnecting in ${wsBackoff}ms…`);
    setTimeout(connectWs, wsBackoff);
    wsBackoff = Math.min(wsBackoff * 2, 5000);
  });

  ws.on('error', (err) => {
    wsReady = false;
    console.error('[WS] Error:', err.message);
    // 'close' will fire after error and trigger reconnect
  });
}

// ── Serial connection ─────────────────────────────────────────────────────────
async function connectSerial() {
  const portPath = await findSerialPort();
  if (!portPath) {
    console.warn('[SERIAL] No port found — retrying in 3s…');
    setTimeout(connectSerial, 3000);
    return;
  }

  console.log(`[SERIAL] Opening ${portPath} at ${BAUD} baud…`);
  serial = new SerialPort({ path: portPath, baudRate: BAUD, autoOpen: false });
  const parser = serial.pipe(new ReadlineParser({ delimiter: '\n' }));

  serial.open((err) => {
    if (err) {
      console.error('[SERIAL] Open error:', err.message, '— retrying in 3s');
      setTimeout(connectSerial, 3000);
      return;
    }
    serialReady = true;
    serialWriter = serial;
    console.log(`[SERIAL] Connected to ${portPath}`);
    printStatus();
  });

  parser.on('data', (line) => {
    line = line.trim();
    if (!line.startsWith('{')) return; // skip non-JSON lines
    console.log('[→phone]', line);
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(line);
    }
  });

  serial.on('close', () => {
    serialReady = false;
    serialWriter = null;
    console.log('[SERIAL] Port closed — retrying in 3s…');
    setTimeout(connectSerial, 3000);
  });

  serial.on('error', (err) => {
    console.error('[SERIAL] Error:', err.message);
  });
}

function printStatus() {
  const s = serialReady ? 'OK' : 'waiting';
  const w = wsReady ? 'OK' : 'waiting';
  console.log(`[STATUS] Serial=${s}  WebSocket=${w}`);
  if (serialReady && wsReady) {
    console.log('[READY] Bridge is live — Uno data flowing to phone via Cloudflare');
  }
}

// ── Boot ──────────────────────────────────────────────────────────────────────
console.log('RhythmRide bridge starting…');
connectWs();
connectSerial();
