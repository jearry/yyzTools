<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.en.md">English</a> · <a href="README.zh-CN.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <strong>Français</strong> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.3.1300-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-MIT-success">
</p>

---

## Présentation

Rechercher, traduire, reconnaître, prévisualiser, traiter par lots, capturer, décorer — toutes ces petites tâches quotidiennes, sans devoir dénicher, installer et payer un outil différent pour chacune. Installez yyzTools une fois : ce dont vous avez besoin est déjà là.

## Points forts

- **Gratuit à vie** — Toutes les fonctions sont ouvertes : aucun déblocage payant, aucun abonnement, et jamais de version bridée destinée à vous faire payer les fonctions essentielles.
- **Priorité au local** — Presse-papiers, recherche de fichiers, statistiques d’usage, OCR : tout est traité et conservé sur votre propre ordinateur, rien n’est envoyé de lui-même.
- **Une seule installation** — Plus de 40 modules prêts à l’emploi, ce qui vous épargne le cycle habituel : chercher, comparer, télécharger, essayer, désinstaller.
- **379 modules de commandes** — Cinq catégories — IA, applications, jeux, système, sites web — disponibles immédiatement, et un simple fichier JSON suffit pour en ajouter.

## Fonctions

| | |
|---|---|
| **Palette de commandes** | Recherche d’applications · Recherche de fichiers · Gestion des processus · Gestion des fenêtres · Recherche de favoris · Historique du presse-papiers · Ligne de commande · Calcul instantané |
| **Lancement et productivité** | Dock · Historique du presse-papiers · Statistiques d’applications · Capture d’écran · Enregistrement d’écran · Assistant de rappels · Calendrier perpétuel · Indication des touches · Localisation de la souris |
| **Texte et langues** | Traduction avancée · Reconnaissance de texte OCR · Expressions régulières · Encodage et décodage · Calcul de hachage · Chiffrement et déchiffrement · Conversion de données · Génération de données |
| **Fichiers et médias** | Aperçu de fichiers · Renommage par lots · Traitement d’images par lots · Traitement vidéo par lots · Traitement PDF par lots · Téléchargement de fichiers |
| **Calcul et extensions** | Bloc de calcul · QR code · Modules de commandes · Navigateur intégré · Extensions personnalisées |
| **Personnalisation** | Fond d’écran animé · Effets de bureau · Mode jeu · Page de navigation · Défense de la Terre |

## Architecture

Hybrid: a **C++ (Win32 + WebView2) host** carrying a **front end (Alpine.js + vanilla JS + Vite)**, with feature areas isolated into separate processes.

| Process | Role |
|---------|------|
| `yyzTools.exe` | Main process — hosts the command palette, translate, OCR and most features |
| `yyzWallpaper.exe` | Live wallpaper — composites web/video under the desktop icon layer via the Windows Composition API |
| `yyzBrowser.exe` | Frameless built-in browser for command modules opening URLs |
| `yyzCmd.exe` | Pure Win32 command runner (shutdown / restart / volume / display / recycle bin, etc.) |
| `yyzInputHint.exe` | Live keyboard / mouse key overlay (RawInput) |
| `yyzMouseFinder.exe` | Quickly locate the mouse cursor |
| `yyzUpdater.exe` | Incremental auto-updater (see `src/updater/`) |

The front end calls each C++ Manager through the `window.Zen` bridge, all returning an `{ error, ... }` contract.

## Download

- **Installer** (recommended): `yyzTools-setup-1.0.3.1300.exe` — [GitHub Releases](https://github.com/jearry/yyzTools/releases)
- Slow from China? Prefix the installer URL with a mirror: [ghfast.top](https://ghfast.top/) · [ghproxy.com](https://ghproxy.com/) · [gh-proxy.com](https://gh-proxy.com/)
- Or grab it from the [official download page](https://yyztools.com/download.html).

The setup wizard supports 12 languages.

## System requirements

- Windows 10 / 11 (x64)
- WebView2 Runtime (preinstalled on recent systems; the installer pulls it in if missing)

## UI languages

Both the UI and command-module text support 12 languages: 简体中文, 繁體中文, English, 日本語, 한국어, Français, Deutsch, Español, Русский, العربية, Português, Italiano. First launch auto-detects the system language; switch anytime in Settings.

## About this repo

This repo (`jearry/yyzTools`) hosts releases and the official website:

- [`releases/`](releases/) — versioned installers and `.7z` packages, plus the `update.json` manifest
- [`docs/`](docs/) — static site for <https://yyztools.com> (GitHub Pages)
- [`src/updater/`](src/updater/) — auto-updater source

> The main application source is not in this repo. For feature issues, please use [Issues](https://github.com/jearry/yyzTools/issues).

## License

[MIT License](LICENSE)
