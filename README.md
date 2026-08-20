<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><strong>English</strong> · <a href="README.zh-CN.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.fr.md">Français</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.3.1300-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-MIT-success">
</p>

---

## Introduction

Search, translate, recognise, preview, batch-process, capture, decorate — the frequent little jobs of everyday work, without hunting down, installing, and paying for a separate tool for each one. Install yyzTools once and whatever you need is already there.

## Highlights

- **Free forever** — Every feature is open. No paid unlocks, no membership tiers, and no crippling the core to sell it back to you.
- **Local-first** — Clipboard, file search, usage stats, and OCR are all processed and stored on your own machine. Nothing is uploaded on its own.
- **Install once** — 40+ modules ready out of the box — skipping the whole cycle of searching, comparing, downloading, trialling, and uninstalling.
- **379 command modules** — AI, apps, games, system, and web commands on tap — and one JSON file is all it takes to add your own.

## Features

| | |
|---|---|
| **CmdPlate** | App search · File search · Process manager · Window manager · Bookmark search · Clipboard history · Command line · Instant calculator |
| **Speed & efficiency** | Dock · Clipboard history · App usage · Screen capture · Screen recorder · Reminders · Calendar · Input hints · Mouse finder |
| **Text & language** | Super translate · OCR · Regex tester · Encode / decode · Hash calculator · Encryption · Data conversion · Data generator |
| **Files & media** | File preview · Batch rename · Batch image tool · Batch video tool · Batch PDF tool · Downloader |
| **Compute & extend** | Calculator pad · QR codes · Command modules · Built-in browser · Custom extensions |
| **Personalisation** | Live wallpaper · Desktop effects · Game mode · Navigation page · Earth Defender |

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
