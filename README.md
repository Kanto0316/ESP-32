# ESP32 Bonjour Wi-Fi

Ce projet contient un sketch Arduino pour ESP32 qui :

- crée un point d'accès Wi-Fi nommé `ESP-32` ;
- utilise le mot de passe `123456789` ;
- affiche `Bonjour` sur le moniteur série ;
- affiche `Bonjour` dans un navigateur en visitant l'adresse IP de l'ESP32.

## Utilisation

1. Ouvrez `esp32_bonjour.ino` dans l'IDE Arduino.
2. Sélectionnez une carte ESP32.
3. Téléversez le sketch.
4. Connectez-vous au Wi-Fi `ESP-32` avec le mot de passe `123456789`.
5. Ouvrez l'adresse `192.168.4.1` pour voir le message `Bonjour`.
