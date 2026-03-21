#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

constexpr char WIFI_SSID[] = "ESP32-CAM";
constexpr char WIFI_PASSWORD[] = "12345678";
constexpr uint16_t HTTP_PORT = 80;
constexpr size_t MAX_TRACKED_CLIENTS = 12;

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

struct ClientRecord {
  uint32_t id = 0;
  bool connected = false;
  String role;
};

ClientRecord clientRecords[MAX_TRACKED_CLIENTS];
uint32_t broadcasterClientId = 0;

String jsonEscape(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);

  for (size_t index = 0; index < value.length(); ++index) {
    const char ch = value[index];

    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += ch;
        break;
    }
  }

  return escaped;
}

ClientRecord* findClientRecord(uint32_t clientId) {
  for (size_t index = 0; index < MAX_TRACKED_CLIENTS; ++index) {
    if (clientRecords[index].connected && clientRecords[index].id == clientId) {
      return &clientRecords[index];
    }
  }

  return nullptr;
}

ClientRecord* ensureClientRecord(uint32_t clientId) {
  ClientRecord* existing = findClientRecord(clientId);
  if (existing != nullptr) {
    return existing;
  }

  for (size_t index = 0; index < MAX_TRACKED_CLIENTS; ++index) {
    if (!clientRecords[index].connected) {
      clientRecords[index].connected = true;
      clientRecords[index].id = clientId;
      clientRecords[index].role = "";
      return &clientRecords[index];
    }
  }

  return nullptr;
}

void removeClientRecord(uint32_t clientId) {
  for (size_t index = 0; index < MAX_TRACKED_CLIENTS; ++index) {
    if (clientRecords[index].connected && clientRecords[index].id == clientId) {
      clientRecords[index].connected = false;
      clientRecords[index].id = 0;
      clientRecords[index].role = "";
      return;
    }
  }
}

String getClientRole(uint32_t clientId) {
  ClientRecord* record = findClientRecord(clientId);
  return record == nullptr ? String("") : record->role;
}

void setClientRole(uint32_t clientId, const String& role) {
  ClientRecord* record = ensureClientRecord(clientId);
  if (record != nullptr) {
    record->role = role;
  }
}

size_t countViewers() {
  size_t viewers = 0;
  for (size_t index = 0; index < MAX_TRACKED_CLIENTS; ++index) {
    if (clientRecords[index].connected && clientRecords[index].role == "viewer") {
      ++viewers;
    }
  }
  return viewers;
}

String buildStatusJson() {
  String payload = "{";
  payload += "\"type\":\"status\",";
  payload += "\"broadcasterOnline\":";
  payload += broadcasterClientId == 0 ? "false" : "true";
  payload += ",\"broadcasterId\":";
  payload += broadcasterClientId;
  payload += ",\"viewerCount\":";
  payload += countViewers();
  payload += "}";
  return payload;
}

void broadcastStatus() {
  ws.textAll(buildStatusJson());
}

void sendJsonToClient(uint32_t clientId, const String& payload) {
  AsyncWebSocketClient* client = ws.client(clientId);
  if (client != nullptr && client->status() == WS_CONNECTED) {
    client->text(payload);
  }
}

void sendError(uint32_t clientId, const String& message) {
  String payload = "{";
  payload += "\"type\":\"error\",\"message\":\"";
  payload += jsonEscape(message);
  payload += "\"}";
  sendJsonToClient(clientId, payload);
}

String extractStringField(const String& json, const char* key) {
  String pattern = String("\"") + key + "\"";
  int keyPosition = json.indexOf(pattern);
  if (keyPosition < 0) {
    return "";
  }

  int colonPosition = json.indexOf(':', keyPosition + pattern.length());
  if (colonPosition < 0) {
    return "";
  }

  int firstQuote = json.indexOf('"', colonPosition + 1);
  if (firstQuote < 0) {
    return "";
  }

  String value;
  bool escaped = false;
  for (int index = firstQuote + 1; index < json.length(); ++index) {
    const char ch = json[index];

    if (escaped) {
      switch (ch) {
        case 'n':
          value += '\n';
          break;
        case 'r':
          value += '\r';
          break;
        case 't':
          value += '\t';
          break;
        default:
          value += ch;
          break;
      }
      escaped = false;
      continue;
    }

    if (ch == '\\') {
      escaped = true;
      continue;
    }

    if (ch == '"') {
      return value;
    }

    value += ch;
  }

  return "";
}

uint32_t extractUintField(const String& json, const char* key) {
  String pattern = String("\"") + key + "\"";
  int keyPosition = json.indexOf(pattern);
  if (keyPosition < 0) {
    return 0;
  }

  int colonPosition = json.indexOf(':', keyPosition + pattern.length());
  if (colonPosition < 0) {
    return 0;
  }

  int start = colonPosition + 1;
  while (start < json.length() && (json[start] == ' ' || json[start] == '\t')) {
    ++start;
  }

  int end = start;
  while (end < json.length() && isDigit(json[end])) {
    ++end;
  }

  if (start == end) {
    return 0;
  }

  return static_cast<uint32_t>(json.substring(start, end).toInt());
}

void relaySessionDescription(const String& type, uint32_t senderId, uint32_t targetId, const String& sdp) {
  String payload = "{";
  payload += "\"type\":\"";
  payload += type;
  payload += "\",\"fromId\":";
  payload += senderId;
  payload += ",\"sdp\":\"";
  payload += jsonEscape(sdp);
  payload += "\"}";
  sendJsonToClient(targetId, payload);
}

void relayIceCandidate(uint32_t senderId, uint32_t targetId, const String& candidate) {
  String payload = "{";
  payload += "\"type\":\"ice-candidate\",\"fromId\":";
  payload += senderId;
  payload += ",\"candidate\":\"";
  payload += jsonEscape(candidate);
  payload += "\"}";
  sendJsonToClient(targetId, payload);
}

void notifyPeerLeft(uint32_t peerId) {
  String payload = "{";
  payload += "\"type\":\"peer-left\",\"peerId\":";
  payload += peerId;
  payload += "}";
  ws.textAll(payload);
}

void registerRole(uint32_t clientId, const String& role) {
  const String previousRole = getClientRole(clientId);

  if (previousRole == "camera" && role != "camera" && broadcasterClientId == clientId) {
    broadcasterClientId = 0;
    notifyPeerLeft(clientId);
  }

  if (role == "camera") {
    if (broadcasterClientId != 0 && broadcasterClientId != clientId) {
      sendError(clientId, "A camera is already active. Disconnect it before starting another one.");
      return;
    }

    broadcasterClientId = clientId;
    setClientRole(clientId, "camera");
  } else if (role == "viewer") {
    setClientRole(clientId, "viewer");
  } else {
    sendError(clientId, "Unknown role requested.");
    return;
  }

  String payload = "{";
  payload += "\"type\":\"registered\",\"role\":\"";
  payload += role;
  payload += "\"}";
  sendJsonToClient(clientId, payload);
  broadcastStatus();
}

void handleViewerReady(uint32_t clientId) {
  if (getClientRole(clientId) != "viewer") {
    sendError(clientId, "Only viewers can request a stream.");
    return;
  }

  if (broadcasterClientId == 0) {
    sendError(clientId, "No camera is currently online.");
    return;
  }

  String payload = "{";
  payload += "\"type\":\"viewer-join\",\"viewerId\":";
  payload += clientId;
  payload += "}";
  sendJsonToClient(broadcasterClientId, payload);
}

void handleSignalMessage(uint32_t clientId, const String& payload) {
  const String type = extractStringField(payload, "type");

  if (type == "register") {
    registerRole(clientId, extractStringField(payload, "role"));
    return;
  }

  if (type == "viewer-ready") {
    handleViewerReady(clientId);
    return;
  }

  if (type == "offer" || type == "answer") {
    const uint32_t targetId = extractUintField(payload, "targetId");
    if (targetId == 0) {
      sendError(clientId, "Missing targetId for session description.");
      return;
    }

    relaySessionDescription(type, clientId, targetId, extractStringField(payload, "sdp"));
    return;
  }

  if (type == "ice-candidate") {
    const uint32_t targetId = extractUintField(payload, "targetId");
    if (targetId == 0) {
      sendError(clientId, "Missing targetId for ICE candidate.");
      return;
    }

    relayIceCandidate(clientId, targetId, extractStringField(payload, "candidate"));
    return;
  }

  if (type == "leave") {
    String role = getClientRole(clientId);
    if (role == "camera" && broadcasterClientId == clientId) {
      broadcasterClientId = 0;
    }
    setClientRole(clientId, "");
    notifyPeerLeft(clientId);
    broadcastStatus();
    return;
  }

  sendError(clientId, "Unsupported message type.");
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover" />
    <title>ESP32 WebRTC Camera Relay</title>
    <style>
      :root {
        color-scheme: dark;
        --bg: #020617;
        --panel: rgba(15, 23, 42, 0.92);
        --panel-border: rgba(148, 163, 184, 0.16);
        --accent: #22c55e;
        --accent-strong: #14b8a6;
        --accent-muted: rgba(34, 197, 94, 0.18);
        --secondary: #38bdf8;
        --danger: #fb7185;
        --text: #e2e8f0;
        --muted: #94a3b8;
        font-family: Inter, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      }

      * {
        box-sizing: border-box;
      }

      body {
        margin: 0;
        min-height: 100vh;
        color: var(--text);
        background:
          radial-gradient(circle at top, rgba(56, 189, 248, 0.16), transparent 30%),
          radial-gradient(circle at bottom right, rgba(34, 197, 94, 0.18), transparent 32%),
          linear-gradient(180deg, #020617 0%, #0f172a 100%);
      }

      .shell {
        width: min(100%, 960px);
        margin: 0 auto;
        padding: 20px;
      }

      .app {
        background: var(--panel);
        border: 1px solid var(--panel-border);
        border-radius: 28px;
        padding: 24px;
        box-shadow: 0 30px 80px rgba(2, 6, 23, 0.5);
      }

      h1 {
        margin: 0;
        font-size: clamp(1.9rem, 5vw, 3rem);
      }

      .subtitle,
      .hint,
      .small {
        color: var(--muted);
        line-height: 1.55;
      }

      .subtitle {
        margin: 12px 0 0;
      }

      .network-card,
      .status-card,
      .log-card {
        margin-top: 20px;
        padding: 18px;
        border-radius: 20px;
        background: rgba(15, 23, 42, 0.7);
        border: 1px solid rgba(148, 163, 184, 0.12);
      }

      .network-grid,
      .video-grid,
      .action-grid,
      .status-grid {
        display: grid;
        gap: 14px;
      }

      .network-grid,
      .status-grid {
        grid-template-columns: repeat(3, minmax(0, 1fr));
      }

      .video-grid,
      .action-grid {
        margin-top: 20px;
        grid-template-columns: repeat(2, minmax(0, 1fr));
      }

      .chip {
        display: inline-flex;
        align-items: center;
        gap: 10px;
        padding: 10px 14px;
        border-radius: 999px;
        background: rgba(15, 23, 42, 0.88);
        border: 1px solid rgba(148, 163, 184, 0.12);
        font-size: 0.95rem;
      }

      .dot {
        width: 11px;
        height: 11px;
        border-radius: 999px;
        background: var(--danger);
        box-shadow: 0 0 14px rgba(251, 113, 133, 0.65);
      }

      .dot.online {
        background: var(--accent);
        box-shadow: 0 0 14px rgba(34, 197, 94, 0.7);
      }

      .stat {
        padding: 16px;
        border-radius: 18px;
        background: rgba(2, 6, 23, 0.32);
      }

      .stat strong {
        display: block;
        margin-top: 10px;
        font-size: 1.1rem;
        color: white;
      }

      .video-card {
        overflow: hidden;
        border-radius: 24px;
        background: rgba(2, 6, 23, 0.52);
        border: 1px solid rgba(148, 163, 184, 0.12);
      }

      .video-header {
        padding: 16px 18px 0;
      }

      video {
        width: 100%;
        min-height: 240px;
        max-height: 52vh;
        display: block;
        object-fit: cover;
        background: #000;
        margin-top: 12px;
      }

      .video-footer {
        padding: 14px 18px 18px;
      }

      .actions {
        margin-top: 20px;
        display: grid;
        gap: 12px;
        grid-template-columns: repeat(3, minmax(0, 1fr));
      }

      button {
        border: 0;
        border-radius: 18px;
        padding: 16px 14px;
        font: inherit;
        font-weight: 700;
        color: white;
        cursor: pointer;
        background: linear-gradient(135deg, var(--accent), var(--accent-strong));
      }

      button.secondary {
        background: linear-gradient(135deg, #0f172a, #1e293b);
        border: 1px solid rgba(148, 163, 184, 0.18);
      }

      button.ghost {
        background: rgba(15, 23, 42, 0.86);
        border: 1px dashed rgba(148, 163, 184, 0.28);
      }

      button:disabled {
        opacity: 0.45;
        cursor: not-allowed;
      }

      .log {
        margin: 0;
        padding-left: 18px;
      }

      .log li + li {
        margin-top: 8px;
      }

      code {
        color: #bfdbfe;
      }

      @media (max-width: 800px) {
        .network-grid,
        .status-grid,
        .video-grid,
        .action-grid,
        .actions {
          grid-template-columns: 1fr;
        }

        .app {
          padding: 18px;
          border-radius: 22px;
        }

        video {
          min-height: 220px;
        }
      }
    </style>
  </head>
  <body>
    <div class="shell">
      <main class="app">
        <h1>ESP32 WebRTC Camera Relay</h1>
        <p class="subtitle">
          The ESP32 hosts this page, creates the Wi‑Fi network, and relays WebRTC signaling messages.
          Video never goes through the ESP32: it streams directly between phones over the local network.
        </p>

        <section class="network-card">
          <div class="chip">
            <span class="dot" id="ws-dot"></span>
            <span id="ws-state">Connecting to ESP32 signaling server...</span>
          </div>
          <div class="network-grid">
            <div class="stat">
              Wi‑Fi network
              <strong>ESP32-CAM</strong>
            </div>
            <div class="stat">
              Password
              <strong>12345678</strong>
            </div>
            <div class="stat">
              Page URL
              <strong><code>http://192.168.4.1</code></strong>
            </div>
          </div>
        </section>

        <section class="actions">
          <button id="start-camera" type="button">Start Camera</button>
          <button id="view-camera" class="secondary" type="button">View Camera</button>
          <button id="disconnect" class="ghost" type="button">Disconnect</button>
        </section>

        <section class="status-card">
          <div class="status-grid">
            <div class="stat">
              My client ID
              <strong id="client-id">Waiting...</strong>
            </div>
            <div class="stat">
              Current role
              <strong id="role-state">Idle</strong>
            </div>
            <div class="stat">
              Connected viewers
              <strong id="viewer-count">0</strong>
            </div>
          </div>
          <p class="hint" id="status-text">
            Choose <strong>Start Camera</strong> on Phone A, then open the page on Phone B and tap <strong>View Camera</strong>.
          </p>
        </section>

        <section class="video-grid">
          <article class="video-card">
            <div class="video-header">
              <strong>Phone A · Local camera preview</strong>
              <p class="small">Visible on the device that shares its camera.</p>
            </div>
            <video id="local-video" autoplay playsinline muted></video>
            <div class="video-footer small">If the preview stays black, verify camera permission was granted in the browser.</div>
          </article>

          <article class="video-card">
            <div class="video-header">
              <strong>Phone B · Remote live stream</strong>
              <p class="small">Rendered from the peer-to-peer WebRTC connection.</p>
            </div>
            <video id="remote-video" autoplay playsinline controls></video>
            <div class="video-footer small">The viewer receives video directly from the sender once signaling completes.</div>
          </article>
        </section>

        <section class="log-card">
          <strong>Usage notes</strong>
          <ol class="log">
            <li>Connect all phones to the ESP32 access point <code>ESP32-CAM</code>.</li>
            <li>Open <code>http://192.168.4.1</code> on each phone.</li>
            <li>Phone A taps <strong>Start Camera</strong> and grants camera permission.</li>
            <li>Phone B taps <strong>View Camera</strong> to request the live stream.</li>
            <li>Additional viewers can also tap <strong>View Camera</strong>; each viewer gets its own WebRTC peer connection.</li>
          </ol>
          <p class="small">
            Important browser note: many mobile browsers require a secure origin for <code>getUserMedia()</code>.
            Android browsers are usually the easiest to test on a local ESP32 page; some iPhone browsers may block camera access over plain HTTP.
          </p>
        </section>
      </main>
    </div>

    <script>
      const socket = new WebSocket(`ws://${window.location.host}/ws`);
      const peerConfig = { iceServers: [] };

      const wsDot = document.getElementById('ws-dot');
      const wsStateEl = document.getElementById('ws-state');
      const clientIdEl = document.getElementById('client-id');
      const roleStateEl = document.getElementById('role-state');
      const viewerCountEl = document.getElementById('viewer-count');
      const statusTextEl = document.getElementById('status-text');
      const startCameraButton = document.getElementById('start-camera');
      const viewCameraButton = document.getElementById('view-camera');
      const disconnectButton = document.getElementById('disconnect');
      const localVideo = document.getElementById('local-video');
      const remoteVideo = document.getElementById('remote-video');

      const state = {
        clientId: null,
        role: 'idle',
        broadcasterId: 0,
        broadcasterOnline: false,
        viewerCount: 0,
        requestedViewerJoin: false,
      };

      let localStream = null;
      let viewerPeer = null;
      const senderPeers = new Map();

      function setStatus(message) {
        statusTextEl.textContent = message;
      }

      function updateConnectionBadge(connected) {
        wsDot.classList.toggle('online', connected);
        wsStateEl.textContent = connected
          ? 'Connected to ESP32 signaling server'
          : 'Disconnected from ESP32 signaling server';
      }

      function renderState() {
        clientIdEl.textContent = state.clientId ? String(state.clientId) : 'Waiting...';
        roleStateEl.textContent = state.role;
        viewerCountEl.textContent = String(state.viewerCount);
        disconnectButton.disabled = state.role === 'idle' && !localStream && senderPeers.size === 0 && !viewerPeer;
      }

      function send(message) {
        if (socket.readyState !== WebSocket.OPEN) {
          setStatus('The signaling connection is not ready yet.');
          return;
        }

        socket.send(JSON.stringify(message));
      }

      function stopMediaTracks(stream) {
        if (!stream) {
          return;
        }

        stream.getTracks().forEach((track) => track.stop());
      }

      function closeSenderPeers() {
        for (const peer of senderPeers.values()) {
          peer.close();
        }
        senderPeers.clear();
      }

      function closeViewerPeer() {
        if (viewerPeer) {
          viewerPeer.close();
          viewerPeer = null;
        }
        remoteVideo.srcObject = null;
      }

      async function createSenderPeer(viewerId) {
        if (!localStream) {
          setStatus('Start the camera before a viewer joins.');
          return;
        }

        if (senderPeers.has(viewerId)) {
          return senderPeers.get(viewerId);
        }

        const peer = new RTCPeerConnection(peerConfig);
        senderPeers.set(viewerId, peer);

        localStream.getTracks().forEach((track) => peer.addTrack(track, localStream));

        peer.onicecandidate = ({ candidate }) => {
          if (!candidate) {
            return;
          }

          send({
            type: 'ice-candidate',
            targetId: viewerId,
            candidate: JSON.stringify(candidate),
          });
        };

        peer.onconnectionstatechange = () => {
          if (['failed', 'closed', 'disconnected'].includes(peer.connectionState)) {
            senderPeers.delete(viewerId);
          }
        };

        const offer = await peer.createOffer({ offerToReceiveVideo: false, offerToReceiveAudio: false });
        await peer.setLocalDescription(offer);
        send({ type: 'offer', targetId: viewerId, sdp: offer.sdp });
        setStatus(`Offer sent to viewer ${viewerId}.`);
        return peer;
      }

      async function ensureViewerPeer(sourceId) {
        if (viewerPeer) {
          return viewerPeer;
        }

        const peer = new RTCPeerConnection(peerConfig);
        viewerPeer = peer;

        peer.ontrack = (event) => {
          const [stream] = event.streams;
          remoteVideo.srcObject = stream;
          setStatus(`Receiving live video from camera ${sourceId}.`);
        };

        peer.onicecandidate = ({ candidate }) => {
          if (!candidate || !sourceId) {
            return;
          }

          send({
            type: 'ice-candidate',
            targetId: sourceId,
            candidate: JSON.stringify(candidate),
          });
        };

        peer.onconnectionstatechange = () => {
          if (peer.connectionState === 'connected') {
            setStatus('Viewer connected to the camera stream.');
          }

          if (['failed', 'closed', 'disconnected'].includes(peer.connectionState)) {
            remoteVideo.srcObject = null;
          }
        };

        return peer;
      }

      async function startCameraMode() {
        try {
          if (state.role === 'viewer') {
            send({ type: 'leave' });
          }

          closeViewerPeer();
          state.requestedViewerJoin = false;

          if (!localStream) {
            localStream = await navigator.mediaDevices.getUserMedia({
              audio: false,
              video: {
                facingMode: 'environment',
                width: { ideal: 1280 },
                height: { ideal: 720 },
              },
            });
            localVideo.srcObject = localStream;
          }

          state.role = 'camera';
          renderState();
          send({ type: 'register', role: 'camera' });
          setStatus('Camera started. Waiting for viewers to request the stream.');
        } catch (error) {
          console.error(error);
          setStatus(`Camera access failed: ${error.message}`);
        }
      }

      function maybeRequestViewerJoin() {
        if (state.role !== 'viewer' || state.requestedViewerJoin || !state.broadcasterOnline || !state.broadcasterId) {
          return;
        }

        state.requestedViewerJoin = true;
        send({ type: 'viewer-ready' });
        setStatus(`Stream requested from camera ${state.broadcasterId}. Waiting for offer...`);
      }

      function startViewerMode() {
        if (state.role === 'camera') {
          send({ type: 'leave' });
        }

        closeSenderPeers();
        stopMediaTracks(localStream);
        localStream = null;
        localVideo.srcObject = null;
        state.role = 'viewer';
        state.requestedViewerJoin = false;
        renderState();
        send({ type: 'register', role: 'viewer' });
        setStatus('Viewer mode enabled. Waiting for an active camera...');
      }

      function disconnectEverything() {
        closeSenderPeers();
        closeViewerPeer();
        stopMediaTracks(localStream);
        localStream = null;
        localVideo.srcObject = null;
        remoteVideo.srcObject = null;
        if (state.role !== 'idle') {
          send({ type: 'leave' });
        }
        state.role = 'idle';
        state.requestedViewerJoin = false;
        renderState();
        setStatus('Disconnected. Choose a role to start again.');
      }

      socket.addEventListener('open', () => {
        updateConnectionBadge(true);
        renderState();
      });

      socket.addEventListener('close', () => {
        updateConnectionBadge(false);
        setStatus('Signaling connection lost. Reopen the page if reconnection does not happen automatically.');
      });

      socket.addEventListener('message', async (event) => {
        const message = JSON.parse(event.data);

        switch (message.type) {
          case 'welcome':
            state.clientId = message.clientId;
            renderState();
            break;
          case 'status':
            state.broadcasterOnline = message.broadcasterOnline;
            state.broadcasterId = message.broadcasterId;
            state.viewerCount = message.viewerCount;
            renderState();
            if (state.role === 'viewer') {
              if (!state.broadcasterOnline) {
                state.requestedViewerJoin = false;
                setStatus('No active camera right now. Waiting for Phone A to start streaming.');
              } else {
                maybeRequestViewerJoin();
              }
            }
            break;
          case 'registered':
            state.role = message.role;
            renderState();
            if (message.role === 'viewer') {
              maybeRequestViewerJoin();
            }
            break;
          case 'viewer-join':
            if (state.role === 'camera') {
              await createSenderPeer(message.viewerId);
            }
            break;
          case 'offer': {
            if (state.role !== 'viewer') {
              break;
            }
            const peer = await ensureViewerPeer(message.fromId);
            await peer.setRemoteDescription({ type: 'offer', sdp: message.sdp });
            const answer = await peer.createAnswer();
            await peer.setLocalDescription(answer);
            send({ type: 'answer', targetId: message.fromId, sdp: answer.sdp });
            setStatus(`Offer received from camera ${message.fromId}. Sending answer...`);
            break;
          }
          case 'answer': {
            const peer = senderPeers.get(message.fromId);
            if (peer) {
              await peer.setRemoteDescription({ type: 'answer', sdp: message.sdp });
              setStatus(`Viewer ${message.fromId} accepted the WebRTC connection.`);
            }
            break;
          }
          case 'ice-candidate': {
            const candidate = JSON.parse(message.candidate);
            if (state.role === 'camera') {
              const peer = senderPeers.get(message.fromId);
              if (peer) {
                await peer.addIceCandidate(candidate);
              }
            } else if (state.role === 'viewer' && viewerPeer) {
              await viewerPeer.addIceCandidate(candidate);
            }
            break;
          }
          case 'peer-left':
            if (state.role === 'camera') {
              const peer = senderPeers.get(message.peerId);
              if (peer) {
                peer.close();
                senderPeers.delete(message.peerId);
                setStatus(`Viewer ${message.peerId} disconnected.`);
              }
            }

            if (state.role === 'viewer' && message.peerId === state.broadcasterId) {
              closeViewerPeer();
              state.requestedViewerJoin = false;
              setStatus('The camera disconnected. Waiting for it to return.');
            }
            break;
          case 'error':
            state.requestedViewerJoin = false;
            setStatus(message.message);
            break;
          default:
            console.warn('Unknown message from ESP32:', message);
        }
      });

      startCameraButton.addEventListener('click', startCameraMode);
      viewCameraButton.addEventListener('click', startViewerMode);
      disconnectButton.addEventListener('click', disconnectEverything);

      renderState();
    </script>
  </body>
</html>
)rawliteral";

void setupAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.println();
  Serial.println("ESP32 WebRTC signaling server started");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("Password: ");
  Serial.println(WIFI_PASSWORD);
  Serial.print("Open: http://");
  Serial.println(WiFi.softAPIP());
}

void handleWebSocketData(void* arg, uint8_t* data, size_t len, uint32_t clientId) {
  AwsFrameInfo* info = static_cast<AwsFrameInfo*>(arg);
  if (info == nullptr || !info->final || info->index != 0 || info->len != len || info->opcode != WS_TEXT) {
    return;
  }

  String payload;
  payload.reserve(len);
  for (size_t index = 0; index < len; ++index) {
    payload += static_cast<char>(data[index]);
  }

  handleSignalMessage(clientId, payload);
}

void onWebSocketEvent(AsyncWebSocket* socket, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (client == nullptr) {
    return;
  }

  const uint32_t clientId = client->id();

  switch (type) {
    case WS_EVT_CONNECT: {
      ensureClientRecord(clientId);
      String payload = "{";
      payload += "\"type\":\"welcome\",\"clientId\":";
      payload += clientId;
      payload += "}";
      client->text(payload);
      client->text(buildStatusJson());
      break;
    }
    case WS_EVT_DISCONNECT: {
      const String role = getClientRole(clientId);
      if (role == "camera" && broadcasterClientId == clientId) {
        broadcasterClientId = 0;
      }
      removeClientRecord(clientId);
      notifyPeerLeft(clientId);
      broadcastStatus();
      break;
    }
    case WS_EVT_DATA:
      handleWebSocketData(arg, data, len, clientId);
      break;
    default:
      break;
  }
}

void setupWebServer() {
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/health", HTTP_GET, [](AsyncWebServerRequest* request) {
    String payload = "{";
    payload += "\"status\":\"ok\",";
    payload += "\"mode\":\"webrtc-signaling\",";
    payload += "\"ssid\":\"";
    payload += WIFI_SSID;
    payload += "\",";
    payload += "\"broadcasterOnline\":";
    payload += broadcasterClientId == 0 ? "false" : "true";
    payload += ",\"viewerCount\":";
    payload += countViewers();
    payload += "}";
    request->send(200, "application/json", payload);
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  setupAccessPoint();
  setupWebServer();
}

void loop() {
  ws.cleanupClients();
}
