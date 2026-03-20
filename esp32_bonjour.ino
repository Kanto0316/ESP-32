#include <WiFi.h>
#include <WebServer.h>

constexpr char WIFI_SSID[] = "ESP-32";
constexpr char WIFI_PASSWORD[] = "123456789";

WebServer server(80);

void handleRoot() {
  server.send(200, "text/html; charset=utf-8",
              "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>ESP32</title></head>"
              "<body><h1>Bonjour</h1></body></html>");
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
