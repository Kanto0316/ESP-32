#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

constexpr char WIFI_SSID[] = "ESP32-COUNTER";
constexpr char WIFI_PASSWORD[] = "12345678";
constexpr uint16_t HTTP_PORT = 80;
constexpr unsigned long COUNT_DELAY_MS = 200;

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

unsigned long count = 0;
bool running = true;
unsigned long startedAtMs = 0;
unsigned long accumulatedElapsedMs = 0;
unsigned long lastCountUpdateMs = 0;

String buildStateJson() {
  const unsigned long elapsedMs = accumulatedElapsedMs + (running ? millis() - startedAtMs : 0);

  String payload = "{";
  payload += "\"count\":";
  payload += count;
  payload += ",\"running\":";
  payload += running ? "true" : "false";
  payload += ",\"delayMs\":";
  payload += COUNT_DELAY_MS;
  payload += ",\"elapsedMs\":";
  payload += elapsedMs;
  payload += "}";
  return payload;
}

void broadcastState() {
  ws.textAll(buildStateJson());
}

void resetCounter() {
  count = 0;
  accumulatedElapsedMs = 0;
  startedAtMs = millis();
  lastCountUpdateMs = millis();
}

void setRunning(bool nextRunning) {
  if (running == nextRunning) {
    return;
  }

  if (nextRunning) {
    startedAtMs = millis();
    lastCountUpdateMs = millis();
  } else {
    accumulatedElapsedMs += millis() - startedAtMs;
  }

  running = nextRunning;
}

void handleCommand(const String& command) {
  if (command == "toggle") {
    setRunning(!running);
  } else if (command == "reset") {
    resetCounter();
  } else if (command == "boost") {
    count += 10;
  }

  broadcastState();
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Compteur rapide ESP32</title>
    <style>
      :root {
        color-scheme: dark;
        --bg: #020617;
        --panel: rgba(15, 23, 42, 0.92);
        --accent: #22d3ee;
        --accent-strong: #0ea5e9;
        --text: #e2e8f0;
        --muted: #94a3b8;
        --success: #22c55e;
        font-family: Inter, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      }

      * {
        box-sizing: border-box;
      }

      body {
        margin: 0;
        min-height: 100vh;
        display: grid;
        place-items: center;
        padding: 20px;
        color: var(--text);
        background:
          radial-gradient(circle at top, rgba(34, 211, 238, 0.24), transparent 30%),
          radial-gradient(circle at bottom right, rgba(14, 165, 233, 0.2), transparent 28%),
          linear-gradient(180deg, #020617 0%, #0f172a 100%);
      }

      .app {
        width: min(100%, 760px);
        background: var(--panel);
        border: 1px solid rgba(148, 163, 184, 0.16);
        border-radius: 28px;
        padding: 28px;
        box-shadow: 0 24px 80px rgba(2, 6, 23, 0.45);
      }

      h1 {
        margin: 0;
        font-size: clamp(1.9rem, 5vw, 3rem);
      }

      .subtitle {
        margin: 10px 0 0;
        color: var(--muted);
        line-height: 1.5;
      }

      .hero {
        margin-top: 24px;
        display: grid;
        gap: 18px;
      }

      .counter-card {
        padding: 28px;
        border-radius: 24px;
        text-align: center;
        background: linear-gradient(145deg, rgba(8, 47, 73, 0.9), rgba(15, 23, 42, 0.95));
        border: 1px solid rgba(34, 211, 238, 0.22);
      }

      .label {
        text-transform: uppercase;
        letter-spacing: 0.18em;
        color: var(--muted);
        font-size: 0.82rem;
      }

      .count {
        margin: 16px 0;
        font-size: clamp(4rem, 16vw, 8rem);
        font-weight: 800;
        line-height: 1;
        color: white;
      }

      .timer {
        display: inline-flex;
        align-items: center;
        gap: 10px;
        padding: 10px 16px;
        border-radius: 999px;
        background: rgba(15, 23, 42, 0.72);
        border: 1px solid rgba(148, 163, 184, 0.14);
      }

      .dot {
        width: 12px;
        height: 12px;
        border-radius: 50%;
        background: var(--success);
        box-shadow: 0 0 12px rgba(34, 197, 94, 0.75);
      }

      .controls {
        display: grid;
        grid-template-columns: repeat(3, minmax(0, 1fr));
        gap: 12px;
      }

      button {
        border: 0;
        border-radius: 16px;
        padding: 16px;
        font: inherit;
        font-weight: 700;
        cursor: pointer;
        color: white;
        background: linear-gradient(135deg, var(--accent), var(--accent-strong));
      }

      button.secondary {
        background: rgba(30, 41, 59, 0.9);
        border: 1px solid rgba(148, 163, 184, 0.18);
      }

      .info-grid {
        display: grid;
        grid-template-columns: repeat(2, minmax(0, 1fr));
        gap: 12px;
      }

      .info-box {
        padding: 18px;
        border-radius: 18px;
        background: rgba(15, 23, 42, 0.78);
        border: 1px solid rgba(148, 163, 184, 0.12);
      }

      .info-box strong {
        display: block;
        margin-top: 8px;
        font-size: 1.2rem;
      }

      .footer-note {
        margin-top: 18px;
        color: var(--muted);
        text-align: center;
      }

      @media (max-width: 640px) {
        .app {
          padding: 20px;
        }

        .controls,
        .info-grid {
          grid-template-columns: 1fr;
        }
      }
    </style>
  </head>
  <body>
    <main class="app">
      <h1>Compteur rapide ESP32</h1>
      <p class="subtitle">
        Cette version du projet a été entièrement transformée pour afficher un comptage automatique
        partagé entre tous les appareils connectés, avec un intervalle fixe de <strong>200 ms</strong>
        entre chaque valeur.
      </p>

      <section class="hero">
        <article class="counter-card">
          <div class="label">Valeur courante</div>
          <div class="count" id="count">0</div>
          <div class="timer">
            <span class="dot"></span>
            <span id="status">Connexion en cours...</span>
          </div>
        </article>

        <div class="controls">
          <button id="toggle-button" type="button">Pause</button>
          <button id="reset-button" class="secondary" type="button">Réinitialiser</button>
          <button id="boost-button" class="secondary" type="button">+10</button>
        </div>

        <div class="info-grid">
          <div class="info-box">
            Intervalle configuré
            <strong>200 ms</strong>
          </div>
          <div class="info-box">
            Temps écoulé
            <strong id="elapsed">0.0 s</strong>
          </div>
        </div>
      </section>

      <p class="footer-note">Connectez-vous au Wi-Fi <strong>ESP32-COUNTER</strong> puis ouvrez <strong>192.168.4.1</strong>.</p>
    </main>

    <script>
      const countEl = document.getElementById('count');
      const statusEl = document.getElementById('status');
      const elapsedEl = document.getElementById('elapsed');
      const toggleButtonEl = document.getElementById('toggle-button');
      const resetButtonEl = document.getElementById('reset-button');
      const boostButtonEl = document.getElementById('boost-button');

      const COUNT_DELAY_MS = 200;
      const socket = new WebSocket(`ws://${window.location.host}/ws`);

      const state = {
        count: 0,
        running: true,
        elapsedMs: 0,
      };

      function render() {
        countEl.textContent = state.count.toString();
        elapsedEl.textContent = `${(state.elapsedMs / 1000).toFixed(1)} s`;
        statusEl.textContent = state.running
          ? `Comptage partagé actif · ${COUNT_DELAY_MS} ms entre chaque nombre`
          : 'Comptage partagé en pause';
        toggleButtonEl.textContent = state.running ? 'Pause' : 'Reprendre';
      }

      function sendCommand(command) {
        if (socket.readyState === WebSocket.OPEN) {
          socket.send(command);
        }
      }

      socket.addEventListener('open', () => {
        render();
      });

      socket.addEventListener('message', (event) => {
        const nextState = JSON.parse(event.data);
        state.count = nextState.count;
        state.running = nextState.running;
        state.elapsedMs = nextState.elapsedMs;
        render();
      });

      socket.addEventListener('close', () => {
        statusEl.textContent = 'Connexion perdue avec l\'ESP32';
      });

      toggleButtonEl.addEventListener('click', () => sendCommand('toggle'));
      resetButtonEl.addEventListener('click', () => sendCommand('reset'));
      boostButtonEl.addEventListener('click', () => sendCommand('boost'));

      render();
    </script>
  </body>
</html>
)rawliteral";

void setupAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

  const IPAddress accessPointIp = WiFi.softAPIP();
  Serial.println();
  Serial.println("Compteur rapide ESP32 demarre");
  Serial.print("SSID : ");
  Serial.println(WIFI_SSID);
  Serial.print("Mot de passe : ");
  Serial.println(WIFI_PASSWORD);
  Serial.print("Intervalle entre deux comptes : ");
  Serial.print(COUNT_DELAY_MS);
  Serial.println(" ms");
  Serial.print("Ouvrez : http://");
  Serial.println(accessPointIp);
}

void handleWebSocketMessage(void* arg, uint8_t* data, size_t len) {
  AwsFrameInfo* info = static_cast<AwsFrameInfo*>(arg);

  if (info == nullptr || !info->final || info->index != 0 || info->len != len || info->opcode != WS_TEXT) {
    return;
  }

  String command;
  command.reserve(len);
  for (size_t index = 0; index < len; ++index) {
    command += static_cast<char>(data[index]);
  }

  handleCommand(command);
}

void onWebSocketEvent(AsyncWebSocket* socket, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    client->text(buildStateJson());
    return;
  }

  if (type == WS_EVT_DATA) {
    handleWebSocketMessage(arg, data, len);
  }
}

void setupWebServer() {
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/health", HTTP_GET, [](AsyncWebServerRequest* request) {
    String payload = "{\"status\":\"ok\",\"mode\":\"counter\",\"delayMs\":";
    payload += COUNT_DELAY_MS;
    payload += ",\"sharedState\":true}";
    request->send(200, "application/json", payload);
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  resetCounter();
  setupAccessPoint();
  setupWebServer();
}

void loop() {
  ws.cleanupClients();

  if (!running) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastCountUpdateMs < COUNT_DELAY_MS) {
    return;
  }

  lastCountUpdateMs += COUNT_DELAY_MS;
  count += 1;
  broadcastState();
}
