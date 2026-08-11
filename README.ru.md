<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.en.md">English</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.fr.md">Français</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <strong>Русский</strong> · <a href="README.ar.md">العربية</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.1.1100-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-MIT-success">
</p>

---

## Введение

Поиск, перевод, распознавание, предпросмотр, пакетная обработка, скриншоты, обои — все частые и мелкие задачи повседневной работы, и больше не нужно для каждой из них отдельно искать, устанавливать и оплачивать программу. Установите yyzTools один раз — и всё нужное окажется под рукой.

## Ключевые особенности

- **Бесплатно навсегда** — Все функции открыты: без платной разблокировки, без уровней подписки и без урезания ключевых возможностей ради скрытой оплаты.
- **Сначала локально** — Буфер обмена, поиск файлов, статистика использования и OCR обрабатываются и хранятся на вашем компьютере и никуда не отправляются сами.
- **Одна установка** — Более 40 модулей готовы к работе сразу — без круга поисков, сравнений, загрузок, пробных версий и удалений.
- **379 командных модулей** — Команды пяти категорий: ИИ, приложения, игры, система и веб-сайты. Чтобы добавить свою, достаточно одного файла JSON.

## Возможности

| | |
|---|---|
| **Командная панель** | Поиск приложений · Поиск файлов · Управление процессами · Управление окнами · Поиск закладок · История буфера обмена · Командная строка · Мгновенный расчёт |
| **Запуск и эффективность** | Док · История буфера обмена · Статистика приложений · Снимок экрана · Помощник напоминаний · Вечный календарь · Подсказки клавиш · Поиск курсора |
| **Текст и язык** | Суперперевод · Распознавание текста OCR · Регулярные выражения · Кодирование и декодирование · Расчёт хешей · Шифрование и расшифровка · Преобразование данных · Генерация данных |
| **Файлы и медиа** | Предпросмотр файлов · Пакетное переименование · Пакетная обработка изображений · Пакетная обработка видео · Пакетная обработка PDF · Загрузка файлов |
| **Вычисления и расширения** | Черновик вычислений · QR-код · Командные модули · Встроенный браузер · Свои расширения |
| **Персонализация** | Динамические обои · Эффекты рабочего стола · Игровой режим · Страница навигации · Защита Земли |

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
