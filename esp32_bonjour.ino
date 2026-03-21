#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

constexpr char WIFI_SSID[] = "ESP32-COUNTER";
constexpr char WIFI_PASSWORD[] = "12345678";
constexpr uint16_t HTTP_PORT = 80;
constexpr unsigned long COUNT_DELAY_MS = 400;

AsyncWebServer server(HTTP_PORT);

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
        très rapide avec un intervalle fixe de <strong>400 ms</strong> entre chaque valeur.
      </p>

      <section class="hero">
        <article class="counter-card">
          <div class="label">Valeur courante</div>
          <div class="count" id="count">0</div>
          <div class="timer">
            <span class="dot"></span>
            <span id="status">Comptage actif · 400 ms entre chaque nombre</span>
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
            <strong>400 ms</strong>
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

      const COUNT_DELAY_MS = 400;
      let count = 0;
      let running = true;
      let countIntervalId = 0;
      let startedAt = Date.now();

      function render() {
        countEl.textContent = count.toString();
        statusEl.textContent = running
          ? `Comptage actif · ${COUNT_DELAY_MS} ms entre chaque nombre`
          : 'Comptage en pause';
        toggleButtonEl.textContent = running ? 'Pause' : 'Reprendre';
      }

      function updateElapsed() {
        const seconds = ((Date.now() - startedAt) / 1000).toFixed(1);
        elapsedEl.textContent = `${seconds} s`;
      }

      function startCounter() {
        window.clearInterval(countIntervalId);
        countIntervalId = window.setInterval(() => {
          count += 1;
          render();
        }, COUNT_DELAY_MS);
      }

      toggleButtonEl.addEventListener('click', () => {
        running = !running;

        if (running) {
          startCounter();
        } else {
          window.clearInterval(countIntervalId);
        }

        render();
      });

      resetButtonEl.addEventListener('click', () => {
        count = 0;
        startedAt = Date.now();
        render();
        updateElapsed();
      });

      boostButtonEl.addEventListener('click', () => {
        count += 10;
        render();
      });

      render();
      startCounter();
      window.setInterval(updateElapsed, 100);
      updateElapsed();
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

void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/health", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{\"status\":\"ok\",\"mode\":\"counter\",\"delayMs\":400}");
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
}
