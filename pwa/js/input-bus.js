/**
 * RhythmRide InputBus — hardware abstraction layer
 *
 * Normalises ALL input sources into one unified event stream.
 * Sources (in priority order, can be mixed):
 *   1. WebSocket  — ESP32 WiFi AP (ws://192.168.4.1:81)
 *   2. WebSerial  — Arduino Nano via USB (Chrome/Edge only)
 *   3. WebBluetooth — ESP32 BLE (future, skeleton ready)
 *   4. PhoneSensors — DeviceMotion stomp detection
 *   5. Keyboard   — always active as fallback
 *   6. Touch      — always active
 *
 * Usage:
 *   const bus = new InputBus();
 *   bus.on('steer',    ({v}) => ...);   // 0.0–1.0
 *   bus.on('btn',      ({i, s}) => ...); // i=0-7 index, s=0|1
 *   bus.on('paddle',   ({i, s}) => ...); // i=0|1
 *   bus.on('stomp',    ({m}) => ...);    // magnitude g
 *   bus.on('jump',     ({m}) => ...);
 *   bus.on('throttle', ({v}) => ...);   // 0.0–1.0
 *   bus.on('brake',    ({v}) => ...);
 *   bus.on('stomps',   ({n}) => ...);   // cumulative count
 *   bus.on('connect',  ({source}) => ...);
 *   bus.on('disconnect',({source}) => ...);
 *   bus.on('status',   ({text, source}) => ...);
 *
 *   bus.send({t:'bpm', v:120});  // sends to all connected hardware
 *   bus.connectWebSocket('ws://192.168.4.1:81');
 *   bus.connectSerial();         // triggers browser file picker
 *   bus.connectBluetooth();      // triggers browser BLE picker
 *   bus.enablePhoneSensors();
 *   bus.enableKeyboard();
 */

class InputBus {
    constructor() {
        this._handlers = {};
        this._ws = null;
        this._wsUrl = null;
        this._wsRetry = null;
        this._serial = null;
        this._serialReader = null;
        this._serialWriter = null;
        this._ble = null;
        this._bleTx = null;
        this._phoneActive = false;
        this._kbActive = false;
        this._steerSim = 0.5;      // keyboard-simulated steering
        this._lastStomp = 0;
        this._serialLineBuf = '';
        this.sources = {ws: false, serial: false, ble: false, phone: false, kb: false};
    }

    // ── Event emitter ─────────────────────────────────────────────────────────
    on(event, fn) {
        (this._handlers[event] = this._handlers[event] || []).push(fn);
        return this;
    }
    off(event, fn) {
        if (this._handlers[event])
            this._handlers[event] = this._handlers[event].filter(h => h !== fn);
    }
    _emit(event, data) {
        (this._handlers[event] || []).forEach(h => h(data));
    }
    _dispatch(msg) {
        if (!msg || !msg.t) return;
        this._emit(msg.t, msg);
    }

    // ── Send to hardware ──────────────────────────────────────────────────────
    send(obj) {
        const s = JSON.stringify(obj);
        if (this._ws && this._ws.readyState === WebSocket.OPEN) {
            this._ws.send(s);
        }
        if (this._serialWriter) {
            this._serialWriter.write(new TextEncoder().encode(s + '\n')).catch(() => {});
        }
        if (this._bleTx) {
            try { this._bleTx.writeValueWithoutResponse(new TextEncoder().encode(s + '\n')); }
            catch (e) {}
        }
    }

    // ══════════════════════════════════════════════════════════════════════════
    // SOURCE 1: WebSocket (ESP32 WiFi)
    // ══════════════════════════════════════════════════════════════════════════
    connectWebSocket(url = 'ws://192.168.4.1:81') {
        this._wsUrl = url;
        this._wsConnect();
    }

    _wsConnect() {
        if (this._ws) { try { this._ws.close(); } catch (e) {} }
        clearTimeout(this._wsRetry);
        this._emit('status', {text: 'WebSocket connecting…', source: 'ws'});

        try {
            this._ws = new WebSocket(this._wsUrl);
        } catch (e) {
            this._scheduleWsRetry(); return;
        }

        this._ws.onopen = () => {
            this.sources.ws = true;
            this._emit('connect', {source: 'ws'});
            this._emit('status', {text: 'WiFi connected (ESP32)', source: 'ws'});
        };
        this._ws.onmessage = ({data}) => {
            try { this._dispatch(JSON.parse(data)); } catch (e) {}
        };
        this._ws.onclose = this._ws.onerror = () => {
            this.sources.ws = false;
            this._emit('disconnect', {source: 'ws'});
            this._scheduleWsRetry();
        };
    }

    _scheduleWsRetry() {
        this._wsRetry = setTimeout(() => this._wsConnect(), 3000);
    }

    disconnectWebSocket() {
        clearTimeout(this._wsRetry);
        if (this._ws) { this._ws.close(); this._ws = null; }
        this.sources.ws = false;
    }

    // ══════════════════════════════════════════════════════════════════════════
    // SOURCE 2: WebSerial (Arduino Nano via USB cable)
    // ══════════════════════════════════════════════════════════════════════════
    async connectSerial(baudRate = 115200) {
        if (!('serial' in navigator)) {
            this._emit('status', {text: 'WebSerial not supported (use Chrome/Edge)', source: 'serial'});
            return false;
        }
        try {
            const port = await navigator.serial.requestPort();
            await port.open({baudRate});
            this._serial = port;
            this._serialWriter = port.writable.getWriter();
            this.sources.serial = true;
            this._emit('connect', {source: 'serial'});
            this._emit('status', {text: 'Serial connected (Arduino Nano)', source: 'serial'});
            this._readSerial(port);
            return true;
        } catch (e) {
            this._emit('status', {text: 'Serial: ' + e.message, source: 'serial'});
            return false;
        }
    }

    async _readSerial(port) {
        const reader = port.readable.getReader();
        this._serialReader = reader;
        const dec = new TextDecoder();
        try {
            while (true) {
                const {value, done} = await reader.read();
                if (done) break;
                this._serialLineBuf += dec.decode(value);
                let nl;
                while ((nl = this._serialLineBuf.indexOf('\n')) !== -1) {
                    const line = this._serialLineBuf.slice(0, nl).trim();
                    this._serialLineBuf = this._serialLineBuf.slice(nl + 1);
                    if (line.startsWith('{')) {
                        try { this._dispatch(JSON.parse(line)); } catch (e) {}
                    }
                }
            }
        } catch (e) {
            // port closed
        } finally {
            reader.releaseLock();
            this.sources.serial = false;
            this._emit('disconnect', {source: 'serial'});
        }
    }

    async disconnectSerial() {
        if (this._serialReader) { try { await this._serialReader.cancel(); } catch (e) {} }
        if (this._serialWriter) { try { this._serialWriter.releaseLock(); } catch (e) {} }
        if (this._serial) { try { await this._serial.close(); } catch (e) {} }
        this._serial = this._serialReader = this._serialWriter = null;
        this.sources.serial = false;
    }

    // ══════════════════════════════════════════════════════════════════════════
    // SOURCE 3: Web Bluetooth (ESP32 BLE — skeleton)
    // ══════════════════════════════════════════════════════════════════════════
    async connectBluetooth() {
        if (!('bluetooth' in navigator)) {
            this._emit('status', {text: 'Web Bluetooth not available', source: 'ble'}); return false;
        }
        const SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';  // Nordic UART
        const RX_UUID      = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';
        const TX_UUID      = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';
        try {
            const device = await navigator.bluetooth.requestDevice({
                filters: [{name: 'RhythmRide'}],
                optionalServices: [SERVICE_UUID]
            });
            this._ble = device;
            device.addEventListener('gattserverdisconnected', () => {
                this.sources.ble = false;
                this._emit('disconnect', {source: 'ble'});
            });
            const server  = await device.gatt.connect();
            const service = await server.getPrimaryService(SERVICE_UUID);
            this._bleTx   = await service.getCharacteristic(RX_UUID);
            const rxChar  = await service.getCharacteristic(TX_UUID);
            await rxChar.startNotifications();
            rxChar.addEventListener('characteristicvaluechanged', e => {
                const s = new TextDecoder().decode(e.target.value).trim();
                if (s.startsWith('{')) try { this._dispatch(JSON.parse(s)); } catch (_) {}
            });
            this.sources.ble = true;
            this._emit('connect', {source: 'ble'});
            this._emit('status', {text: 'BLE connected (ESP32)', source: 'ble'});
            return true;
        } catch (e) {
            this._emit('status', {text: 'BLE: ' + e.message, source: 'ble'}); return false;
        }
    }

    // ══════════════════════════════════════════════════════════════════════════
    // SOURCE 4: Phone sensors (DeviceMotion stomp detection)
    // ══════════════════════════════════════════════════════════════════════════
    enablePhoneSensors() {
        if (this._phoneActive) return;
        this._phoneActive = true;

        const requestPerm = async () => {
            if (typeof DeviceMotionEvent !== 'undefined' &&
                typeof DeviceMotionEvent.requestPermission === 'function') {
                try { await DeviceMotionEvent.requestPermission(); } catch (e) { return; }
            }
            window.addEventListener('devicemotion', this._onMotion.bind(this));
            window.addEventListener('deviceorientation', this._onOrientation.bind(this));
            this.sources.phone = true;
            this._emit('connect', {source: 'phone'});
            this._emit('status', {text: 'Phone sensors active', source: 'phone'});
        };
        requestPerm();
    }

    _onMotion(e) {
        const a = e.accelerationIncludingGravity;
        if (!a) return;
        const mag = Math.sqrt(a.x*a.x + a.y*a.y + a.z*a.z) / 9.81;
        const now = Date.now();
        if (mag > 2.5 && (now - this._lastStomp) > 350) {
            this._lastStomp = now;
            this._dispatch({t: 'stomp', m: +mag.toFixed(1), src: 'phone'});
        }
    }

    _onOrientation(e) {
        // gamma: left-right tilt −90…90 → steer 0.0…1.0
        if (e.gamma == null) return;
        const v = Math.min(1, Math.max(0, (e.gamma + 45) / 90));
        this._dispatch({t: 'steer', v: +v.toFixed(3), src: 'phone'});
    }

    disablePhoneSensors() {
        window.removeEventListener('devicemotion', this._onMotion);
        window.removeEventListener('deviceorientation', this._onOrientation);
        this._phoneActive = false;
        this.sources.phone = false;
    }

    // ══════════════════════════════════════════════════════════════════════════
    // SOURCE 5 & 6: Keyboard + Touch (always available, demo fallback)
    // ══════════════════════════════════════════════════════════════════════════
    enableKeyboard() {
        if (this._kbActive) return;
        this._kbActive = true;
        this.sources.kb = true;

        let steer = 0.5;
        const steerInterval = setInterval(() => {
            this._dispatch({t: 'steer', v: +steer.toFixed(3), src: 'kb'});
        }, 50);

        const onKey = (e, state) => {
            if (e.repeat) return;
            const map = {
                ' ': () => state && this._dispatch({t: 'stomp', m: 2.8, src: 'kb'}),
                'ArrowLeft':  () => { if (state) steer = Math.max(0, steer - 0.1); },
                'ArrowRight': () => { if (state) steer = Math.min(1, steer + 0.1); },
                'ArrowUp':    () => state && this._dispatch({t: 'throttle', v: state ? 0.8 : 0, src: 'kb'}),
                'ArrowDown':  () => state && this._dispatch({t: 'brake',    v: state ? 0.8 : 0, src: 'kb'}),
                'Enter':      () => state && this._dispatch({t: 'jump', m: 3.0, src: 'kb'}),
            };
            // A-H → buttons 0-7
            const code = e.key.toUpperCase();
            if (code >= 'A' && code <= 'H') {
                const i = code.charCodeAt(0) - 65;
                this._dispatch({t: 'btn', i, s: state ? 1 : 0, src: 'kb'});
            }
            if (map[e.key]) { e.preventDefault(); map[e.key](); }
        };

        window.addEventListener('keydown', e => onKey(e, true));
        window.addEventListener('keyup',   e => onKey(e, false));

        this._emit('connect', {source: 'kb'});
        this._emit('status', {text: 'Keyboard active (SPACE=stomp, A-H=pads, ←→=steer, Enter=jump)', source: 'kb'});
    }

    // ── Status helper ─────────────────────────────────────────────────────────
    statusSummary() {
        const active = Object.entries(this.sources)
            .filter(([, v]) => v).map(([k]) => k);
        return active.length ? active.join('+') : 'none';
    }
}

// Export for module use or attach to window
if (typeof module !== 'undefined') module.exports = InputBus;
else window.InputBus = InputBus;
