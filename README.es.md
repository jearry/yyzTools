<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.en.md">English</a> · <a href="README.zh-CN.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.fr.md">Français</a> · <a href="README.de.md">Deutsch</a> · <strong>Español</strong> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.2.1200-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-MIT-success">
</p>

---

## Introducción

Buscar, traducir, reconocer, previsualizar, procesar por lotes, capturar pantalla, personalizar el fondo: todas esas tareas cotidianas, frecuentes y menudas, sin tener que buscar, instalar y pagar una extensión distinta para cada una. Instale yyzTools una vez y tendrá a mano lo que necesite.

## Características

- **Gratis para siempre** — Todas las funciones están abiertas. Sin desbloqueos de pago, sin niveles de suscripción y sin recortar lo esencial para luego cobrárselo.
- **Prioridad local** — El portapapeles, la búsqueda de archivos, las estadísticas de uso y el OCR se procesan y guardan en su propio equipo. Nada se envía por iniciativa propia.
- **Una sola instalación** — Más de 40 módulos listos para usar, sin el habitual ciclo de buscar, comparar, descargar, probar y desinstalar.
- **379 módulos de comandos** — Comandos de IA, aplicaciones, juegos, sistema y sitios web siempre a mano, y basta un archivo JSON para añadir los suyos.

## Funciones

| | |
|---|---|
| **Paleta de comandos** | Búsqueda de aplicaciones · Búsqueda de archivos · Gestión de procesos · Gestión de ventanas · Búsqueda de marcadores · Historial del portapapeles · Línea de comandos · Cálculo instantáneo |
| **Inicio y eficiencia** | Dock · Historial del portapapeles · Estadísticas de uso · Captura de pantalla · Asistente de recordatorios · Calendario perpetuo · Indicaciones de teclado · Localizador del ratón |
| **Texto e idiomas** | Traducción avanzada · Reconocimiento de texto OCR · Expresiones regulares · Codificar y decodificar · Cálculo de hash · Cifrado y descifrado · Conversión de datos · Generación de datos |
| **Archivos y multimedia** | Vista previa de archivos · Renombrado por lotes · Procesamiento por lotes de imágenes · Procesamiento por lotes de vídeo · Procesamiento por lotes de PDF · Descarga de archivos |
| **Cálculo y extensiones** | Hoja de cálculo libre · Códigos QR · Módulos de comandos · Navegador integrado · Extensiones propias |
| **Personalización** | Fondo de pantalla dinámico · Efectos de escritorio · Modo juego · Página de navegación · Defensa de la Tierra |

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

- **Installer** (recommended): `yyzTools-setup-1.0.2.1200.exe` — [GitHub Releases](https://github.com/jearry/yyzTools/releases)
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
