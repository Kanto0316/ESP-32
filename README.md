# Chat WebSocket ESP32

Ce projet contient un système de chat complet pour ESP32, développé avec le **framework Arduino**, **AsyncTCP** et **ESPAsyncWebServer**.

## Fonctionnalités

- L'ESP32 crée son propre point d'accès Wi-Fi :
  - **SSID :** `ESP32-CHAT`
  - **Mot de passe :** `12345678`
- L'ESP32 héberge une interface de chat web adaptée aux mobiles sur `http://192.168.4.1`
- Utilise un **serveur WebSocket** directement hébergé sur l'ESP32
- Plusieurs téléphones ou navigateurs peuvent se connecter en même temps
- Les messages sont diffusés en temps réel à tous les clients connectés
- Chaque utilisateur voit :
  - un numéro de client attribué par l'ESP32
  - un nom d'utilisateur optionnel
  - les nouveaux messages sans recharger la page
  - un défilement automatique de la conversation
- Inclut des notifications système lors de l'arrivée et du départ des clients

## Fichiers

- `esp32_bonjour.ino` - sketch Arduino ESP32 complet
- `index.html` - copie autonome de l'interface web utilisée par le sketch

## Bibliothèques requises

Installez ces bibliothèques dans l'IDE Arduino avant le téléversement :

- **ESPAsyncWebServer**
- **AsyncTCP**

Sélectionnez également une **carte ESP32** dans l'IDE Arduino ou PlatformIO.

## Fonctionnement

1. L'ESP32 démarre en **mode point d'accès**.
2. Les téléphones se connectent au réseau Wi-Fi `ESP32-CHAT`.
3. Le navigateur ouvre `http://192.168.4.1`.
4. La page se connecte au point d'entrée WebSocket `/ws` de l'ESP32.
5. Chaque message envoyé par un client est diffusé en temps réel à tous les autres clients connectés.

## Téléversement

1. Ouvrez `esp32_bonjour.ino` dans l'IDE Arduino.
2. Installez les bibliothèques requises :
   - `ESPAsyncWebServer`
   - `AsyncTCP`
3. Allez dans **Outils > Type de carte** et choisissez votre carte ESP32.
4. Sélectionnez le bon port série.
5. Cliquez sur **Téléverser**.
6. Ouvrez le **Moniteur série** à `115200` bauds.
7. Attendez que l'ESP32 affiche les informations du point d'accès.

## Utilisation

1. Sur votre téléphone, connectez-vous au réseau Wi-Fi :
   - **SSID :** `ESP32-CHAT`
   - **Mot de passe :** `12345678`
2. Ouvrez un navigateur et allez sur :
   - `http://192.168.4.1`
3. Saisissez éventuellement un nom d'utilisateur.
4. Tapez un message puis appuyez sur **Envoyer**.
5. Ouvrez la même page sur d'autres téléphones pour discuter en temps réel.

## Notes

- Le numéro de client est attribué par l'ESP32 lorsqu'un navigateur se connecte.
- Les horodatages sont affichés dans l'interface du navigateur.
- L'interface web est intégrée directement dans le sketch `.ino` et également fournie dans un fichier `index.html` séparé pour faciliter les modifications.
