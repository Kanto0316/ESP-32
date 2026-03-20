#include <WiFi.h>
#include <WebServer.h>

constexpr char WIFI_SSID[] = "ESP-32";
constexpr char WIFI_PASSWORD[] = "123456789";

WebServer server(80);

void handleRoot() {
  server.send(200, "text/html; charset=utf-8",
              R"rawliteral(
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
        padding: 2.5rem 3.5rem;
        border-radius: 1.75rem;
        background: rgba(255, 255, 255, 0.14);
        border: 1px solid rgba(255, 255, 255, 0.22);
        backdrop-filter: blur(18px);
        box-shadow: 0 24px 80px rgba(15, 23, 42, 0.35);
        text-align: center;
      }

      h1 {
        margin: 0;
        font-size: clamp(3rem, 10vw, 5.5rem);
        font-weight: 800;
        letter-spacing: 0.08em;
        text-transform: uppercase;
        color: #f8fafc;
        text-shadow: 0 10px 30px rgba(15, 23, 42, 0.35);
      }
    </style>
  </head>
  <body>
    <main class="card">
      <h1>bonjour</h1>
    </main>
  </body>
</html>
)rawliteral");
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
  Serial.println("Bonjour");

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();
}
