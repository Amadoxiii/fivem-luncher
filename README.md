# FiveM Launcher

Un launcher natif pour **FiveM**, écrit en C pur avec l'API Win32 (pas de framework, pas de dépendances lourdes), doté d'une interface sombre moderne inspirée des outils dev actuels.

## ✨ Fonctionnalités

- **Accueil** — Détection automatique de l'installation FiveM et lancement en un clic
- **Cache** — Visualisation de la taille du cache et nettoyage (normal / extrême)
- **Mods & Plugins** — Activation/désactivation rapide des dossiers `mods` et `plugins`
- **Paramètres** — Sélection de la build GTA V (Automatique ou build spécifique, d'Arena War à A Safehouse in the Hills) et du délai de connexion
- **Serveurs** — Gestionnaire de serveurs favoris (ajout, connexion directe, suppression)

## 🎨 Interface

Interface entièrement dessinée à la main (custom drawing, boutons owner-draw, onglets personnalisés) avec une palette sombre façon GitHub Dark :

- Fond `#0D1117`, panneaux `#161B22`
- Accent orange `#F97316`
- Rendu 100% Win32 (GDI), sans dépendance externe

## 🛠️ Compilation

Projet en C, à compiler avec MinGW ou MSVC.

**Bibliothèques liées :**
- `comctl32.lib`
- `msimg32.lib`
- `shell32.lib`

```bash
gcc main.c -o FiveMLauncher.exe -municode -lcomctl32 -lmsimg32 -lshell32 -mwindows
```

## 📁 Configuration

Le launcher stocke sa configuration à côté de l'exécutable :
- `launcher_servers.ini` — liste des serveurs enregistrés
- config des préférences (build sélectionnée, délai de connexion, etc.)

## ⚠️ Prérequis

- Windows
- FiveM installé (détection automatique via `%LOCALAPPDATA%\FiveM`)
