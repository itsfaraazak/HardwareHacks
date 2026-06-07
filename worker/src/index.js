/**
 * RhythmRide Cloudflare Worker — WebSocket relay via Durable Objects
 *
 * Endpoint: GET /ws?role=bridge   (laptop bridge connects here)
 *           GET /ws?role=phone    (judge's phone connects here)
 *
 * Messages from bridge  → broadcast to ALL phone sessions
 * Messages from phones  → forward to ALL bridge sessions
 */

export default {
  async fetch(request, env) {
    const url = new URL(request.url);

    // CORS preflight
    if (request.method === 'OPTIONS') {
      return corsResponse(new Response(null, { status: 204 }));
    }

    if (url.pathname === '/ws') {
      const id = env.RELAY.idFromName('main');
      const obj = env.RELAY.get(id);
      return obj.fetch(request);
    }

    return corsResponse(new Response('RhythmRide relay OK', { status: 200 }));
  },
};

function corsResponse(res) {
  const headers = new Headers(res.headers);
  headers.set('Access-Control-Allow-Origin', '*');
  headers.set('Access-Control-Allow-Methods', 'GET, OPTIONS');
  headers.set('Access-Control-Allow-Headers', 'Content-Type');
  return new Response(res.body, { status: res.status, headers });
}

// ── Durable Object ────────────────────────────────────────────────────────────
export class Relay {
  constructor(state, env) {
    this.state = state;
    // Sessions tracked via Durable Object WebSocket hibernation API
  }

  async fetch(request) {
    const url = new URL(request.url);
    const role = url.searchParams.get('role') || 'phone';

    if (request.headers.get('Upgrade') !== 'websocket') {
      return new Response('Expected WebSocket upgrade', { status: 426 });
    }

    const [client, server] = Object.values(new WebSocketPair());
    // Tag each socket with its role so we can target them later
    this.state.acceptWebSocket(server, [role]);

    return new Response(null, { status: 101, webSocket: client });
  }

  // Called by the runtime for each incoming WebSocket message
  webSocketMessage(ws, message) {
    const tags = this.state.getTags(ws);
    const senderRole = tags[0]; // 'bridge' or 'phone'

    // bridge → broadcast to all phones
    // phone  → forward to all bridges
    const targetRole = senderRole === 'bridge' ? 'phone' : 'bridge';

    for (const ws2 of this.state.getWebSockets(targetRole)) {
      try { ws2.send(message); } catch (_) {}
    }
  }

  webSocketClose(ws, code, reason) {
    // Hibernation API: runtime handles cleanup automatically
  }

  webSocketError(ws, error) {
    try { ws.close(1011, 'error'); } catch (_) {}
  }
}
