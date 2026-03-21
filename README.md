# Compteur rapide ESP32

Ce projet a été entièrement refait pour transformer l'ESP32 en **borne Wi-Fi affichant un compteur automatique**.

## Fonctionnalités

- L'ESP32 crée son propre point d'accès Wi-Fi :
  - **SSID :** `ESP32-COUNTER`
  - **Mot de passe :** `12345678`
- L'ESP32 héberge une interface web sur `http://192.168.4.1`
- La page affiche un **comptage rapide** qui augmente automatiquement
- Le délai entre chaque incrémentation est fixé à **400 ms**
- L'interface inclut :
  - un affichage géant du nombre courant
  - un bouton **Pause / Reprendre**
  - un bouton **Réinitialiser**
  - un bouton **+10**
  - un affichage du temps écoulé

## Fichiers

- `esp32_bonjour.ino` - sketch Arduino principal avec l'interface web intégrée
- `README.md` - documentation du projet

## Bibliothèques requises

Installez ces bibliothèques dans l'IDE Arduino avant le téléversement :

- **ESPAsyncWebServer**
- **AsyncTCP**

Sélectionnez également une **carte ESP32** compatible dans l'IDE Arduino ou PlatformIO.

## Fonctionnement

1. L'ESP32 démarre en **mode point d'accès**.
2. Un téléphone ou un ordinateur se connecte au réseau Wi-Fi `ESP32-COUNTER`.
3. Le navigateur ouvre `http://192.168.4.1`.
4. La page affiche immédiatement un compteur automatique.
5. Le nombre augmente toutes les **400 millisecondes**.

## Téléversement

1. Ouvrez `esp32_bonjour.ino` dans l'IDE Arduino.
2. Installez les bibliothèques suivantes :
   - `ESPAsyncWebServer`
   - `AsyncTCP`
3. Choisissez votre carte ESP32 dans **Outils > Type de carte**.
4. Sélectionnez le bon port série.
5. Cliquez sur **Téléverser**.
6. Ouvrez le **Moniteur série** à `115200` bauds.

## Utilisation

1. Connectez-vous au réseau Wi-Fi :
   - **SSID :** `ESP32-COUNTER`
   - **Mot de passe :** `12345678`
2. Ouvrez un navigateur à l'adresse suivante :
   - `http://192.168.4.1`
3. Regardez le compteur défiler avec un intervalle de **400 ms**.
4. Utilisez les boutons pour mettre en pause, relancer ou réinitialiser le comptage.

## Vérification rapide

L'endpoint suivant permet de confirmer que le mode compteur est actif :

- `http://192.168.4.1/health`

Il retourne un JSON confirmant le statut du serveur ainsi que la valeur du délai.
