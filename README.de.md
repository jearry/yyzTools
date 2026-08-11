<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.en.md">English</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.fr.md">Français</a> · <strong>Deutsch</strong> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.1.1100-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-MIT-success">
</p>

---

## Überblick

Suchen, übersetzen, erkennen, vorschauen, stapelweise verarbeiten, aufnehmen, gestalten — die häufigen kleinen Aufgaben des Alltags, ohne für jede einzeln einen Plugin-Markt durchzusuchen, zu installieren und zu bezahlen. yyzTools einmal installieren, und was Sie brauchen, ist schon da.

## Highlights

- **Dauerhaft kostenlos** — Alle Funktionen sind offen. Keine kostenpflichtige Freischaltung, keine Mitgliedschaft und kein Beschneiden der Kernfunktionen, um sie Ihnen später zu verkaufen.
- **Lokal zuerst** — Zwischenablage, Dateisuche, Nutzungsstatistiken und OCR werden ausschließlich auf Ihrem eigenen Rechner verarbeitet und gespeichert. Nichts wird von selbst hochgeladen.
- **Einmal installieren** — Über 40 Module sofort einsatzbereit — der ganze Kreislauf aus Suchen, Vergleichen, Herunterladen, Testen und Deinstallieren entfällt.
- **379 Kommandomodule** — Befehle für KI, Apps, Spiele, System und Web jederzeit abrufbar — eine einzige JSON-Datei genügt für eigene Erweiterungen.

## Funktionen

| | |
|---|---|
| **Kommandoleiste** | App-Suche · Dateisuche · Prozessverwaltung · Fensterverwaltung · Lesezeichensuche · Zwischenablage-Verlauf · Kommandozeile · Sofortberechnung |
| **Start und Effizienz** | Dock · Zwischenablage-Verlauf · App-Statistiken · Bildschirmaufnahme · Erinnerungsassistent · Ewiger Kalender · Tastenhinweise · Mauszeigersuche |
| **Text und Sprache** | Super-Übersetzung · Texterkennung OCR · Reguläre Ausdrücke · Kodieren und Dekodieren · Hash-Berechnung · Ver- und Entschlüsselung · Datenkonvertierung · Datengenerierung |
| **Dateien und Medien** | Dateivorschau · Stapelumbenennung · Bild-Stapelverarbeitung · Video-Stapelverarbeitung · PDF-Stapelverarbeitung · Dateidownload |
| **Rechnen und Erweitern** | Rechenblock · QR-Codes · Kommandomodule · Integrierter Browser · Eigene Erweiterungen |
| **Personalisierung** | Dynamischer Hintergrund · Desktop-Effekte · Spielmodus · Navigationsseite · Erdverteidigung |

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

- **Installer** (recommended): `yyzTools-setup-1.0.1.1100.exe` — [GitHub Releases](https://github.com/jearry/yyzTools/releases)
- Slow from China? Prefix the installer URL with a mirror: [ghfast.top](https://ghfast.top/) · [ghproxy.com](https://ghproxy.com/) · [gh-proxy.com](https://gh-proxy.com/)
- Or grab it from the [official download page](https://yyztools.com/en/download.html).

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
