# ESP32 WebRTC camera relay

Ce projet transforme totalement le sketch d'origine en un **serveur de signalisation WebRTC léger pour ESP32**.

L'ESP32 :

- crée un point d'accès Wi‑Fi local ;
- héberge une interface web mobile ;
- relaie les messages WebSocket nécessaires à WebRTC ;
- **ne traite jamais la vidéo**.

La vidéo circule directement entre les téléphones via **WebRTC peer-to-peer**.

## Architecture

- **ESP32 = point d'accès Wi‑Fi + serveur HTTP + serveur WebSocket de signalisation**
- **Téléphone A = émetteur vidéo** avec `getUserMedia()`
- **Téléphone B (et plus) = récepteur vidéo** avec `RTCPeerConnection`
- **Messages échangés via WebSocket :**
  - enregistrement du rôle (`camera` / `viewer`)
  - demande de visualisation
  - `offer` / `answer`
  - candidats ICE
  - déconnexion d'un pair

## Réseau Wi‑Fi

- **SSID :** `ESP32-CAM`
- **Mot de passe :** `12345678`
- **URL locale :** `http://192.168.4.1`

## Fichiers

- `esp32_bonjour.ino` : sketch Arduino complet, avec page HTML intégrée dans le firmware.
- `README.md` : guide d'utilisation.

## Bibliothèques Arduino nécessaires

Installez les bibliothèques suivantes avant compilation :

- `ESPAsyncWebServer`
- `AsyncTCP`
- le support de carte **ESP32** dans l'IDE Arduino ou PlatformIO

## Fonctionnement

1. L'ESP32 démarre en mode **Access Point**.
2. Les smartphones se connectent au Wi‑Fi `ESP32-CAM`.
3. Chaque téléphone ouvre `http://192.168.4.1`.
4. **Phone A** appuie sur **Start Camera** pour publier sa caméra.
5. **Phone B** appuie sur **View Camera** pour recevoir le flux.
6. L'ESP32 relaie les messages WebSocket de signalisation.
7. Une connexion WebRTC directe s'établit entre les deux appareils.
8. D'autres viewers peuvent aussi se connecter : chaque viewer obtient une connexion pair-à-pair dédiée avec le téléphone caméra.

## Utilisation détaillée

### 1. Téléversement

1. Ouvrez `esp32_bonjour.ino` dans l'IDE Arduino.
2. Vérifiez que les bibliothèques `ESPAsyncWebServer` et `AsyncTCP` sont installées.
3. Sélectionnez votre carte ESP32.
4. Choisissez le bon port série.
5. Téléversez le sketch.
6. Ouvrez le moniteur série à `115200` bauds.

### 2. Connexion

1. Connectez les deux smartphones au réseau Wi‑Fi `ESP32-CAM`.
2. Saisissez `http://192.168.4.1` dans le navigateur sur chaque appareil.

### 3. Téléphone caméra

1. Sur le téléphone A, appuyez sur **Start Camera**.
2. Autorisez l'accès à la caméra.
3. Le téléphone devient la **source vidéo**.

### 4. Téléphone viewer

1. Sur le téléphone B, appuyez sur **View Camera**.
2. Le téléphone envoie une demande de stream au téléphone caméra.
3. Quand l'offre WebRTC arrive, le flux s'affiche dans la zone vidéo distante.

## Endpoint de vérification

- `GET /health`

Réponse JSON typique :

```json
{
  "status": "ok",
  "mode": "webrtc-signaling",
  "ssid": "ESP32-CAM",
  "broadcasterOnline": false,
  "viewerCount": 0
}
```

## Limite importante côté navigateur

WebRTC fonctionne bien en réseau local, mais **l'accès caméra via `getUserMedia()` dépend des règles de sécurité du navigateur**.

À savoir :

- beaucoup de navigateurs mobiles exigent un contexte sécurisé pour la caméra ;
- sur certaines plateformes, une page HTTP locale servie par l'ESP32 peut être bloquée pour la capture caméra ;
- en pratique, **Android est généralement plus simple à tester** que certains navigateurs iPhone/iOS sur HTTP local.

Le code fourni implémente l'architecture demandée dans **un seul fichier principal `.ino`** : **ESP32 pour la signalisation, WebRTC pour la vidéo P2P**, sans dépendre d'Internet.
