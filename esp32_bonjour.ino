#include <WiFi.h>
#include <WebServer.h>

constexpr char WIFI_SSID[] = "ESP-32";
constexpr char WIFI_PASSWORD[] = "123456789";
constexpr uint8_t TOUCH_PIN = 4;
constexpr uint16_t TOUCH_THRESHOLD = 30;
constexpr unsigned long DEBOUNCE_DELAY_MS = 250;

WebServer server(80);

uint32_t touchCount = 0;
bool touchActive = false;
unsigned long lastTouchAt = 0;

String renderPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>ESP32</title>
    <style>
      :root {
        color-scheme: light;
        font-family: "Inter", "Segoe UI", sans-serif;
      }

      * {
        box-sizing: border-box;
      }

      body {
        margin: 0;
        min-height: 100vh;
        display: flex;
        align-items: center;
        justify-content: center;
        background: radial-gradient(circle at top, #60a5fa 0%, #2563eb 35%, #0f172a 100%);
        overflow: hidden;
      }

      body::before,
      body::after {
        content: "";
        position: fixed;
        inset: auto;
        width: 18rem;
        height: 18rem;
        border-radius: 50%;
        filter: blur(24px);
        opacity: 0.55;
        z-index: 0;
      }

      body::before {
        top: 8%;
        left: 10%;
        background: rgba(255, 255, 255, 0.28);
      }

      body::after {
        right: 8%;
        bottom: 10%;
        background: rgba(56, 189, 248, 0.3);
      }

      .card {
        position: relative;
        z-index: 1;
        min-width: min(22rem, calc(100vw - 2rem));
        padding: 2.5rem 3.5rem;
        border-radius: 1.75rem;
        background: rgba(255, 255, 255, 0.14);
        border: 1px solid rgba(255, 255, 255, 0.22);
        backdrop-filter: blur(18px);
        box-shadow: 0 24px 80px rgba(15, 23, 42, 0.35);
        text-align: center;
      }

      .label {
        margin: 0 0 0.75rem;
        font-size: 0.95rem;
        font-weight: 600;
        letter-spacing: 0.08em;
        text-transform: uppercase;
        color: rgba(248, 250, 252, 0.8);
      }

      h1 {
        margin: 0;
        font-size: clamp(3rem, 10vw, 5.5rem);
        font-weight: 800;
        letter-spacing: 0.08em;
        color: #f8fafc;
        text-shadow: 0 10px 30px rgba(15, 23, 42, 0.35);
      }

      .hint {
        margin: 1rem 0 0;
        font-size: 0.95rem;
        color: rgba(248, 250, 252, 0.78);
      }
    </style>
  </head>
  <body>
    <main class="card">
      <p class="label">Compteur GPIO4</p>
      <h1 id="counter">0</h1>
      <p class="hint">Touchez GPIO4 pour incrémenter le compteur.</p>
    </main>

    <script>
      async function refreshCounter() {
        try {
          const response = await fetch('/count');
          if (!response.ok) {
            return;
          }

          const data = await response.json();
          document.getElementById('counter').textContent = data.count;
        } catch (error) {
          console.error('Lecture du compteur impossible', error);
        }
      }

      refreshCounter();
      setInterval(refreshCounter, 500);
    </script>
  </body>
</html>
)rawliteral";

  return html;
}

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", renderPage());
}

void handleCount() {
  server.send(200, "application/json", String("{\"count\":") + touchCount + "}");
}

void updateTouchCounter() {
  const unsigned long now = millis();
  const bool isTouched = touchRead(TOUCH_PIN) < TOUCH_THRESHOLD;

  if (isTouched && !touchActive && (now - lastTouchAt >= DEBOUNCE_DELAY_MS)) {
    touchActive = true;
    lastTouchAt = now;
    ++touchCount;
    Serial.print("Compteur GPIO4: ");
    Serial.println(touchCount);
  } else if (!isTouched && touchActive) {
    touchActive = false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

  IPAddress ip = WiFi.softAPIP();

  Serial.println();
  Serial.println("Point d'acces Wi-Fi demarre");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("Mot de passe: ");
  Serial.println(WIFI_PASSWORD);
  Serial.print("Adresse IP: ");
  Serial.println(ip);
  Serial.println("Compteur GPIO4 initialise a 0");

  server.on("/", handleRoot);
  server.on("/count", handleCount);
  server.begin();
}

void loop() {
  updateTouchCounter();
  server.handleClient();
}
