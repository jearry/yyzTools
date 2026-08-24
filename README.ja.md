<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.en.md">English</a> · <a href="README.zh-CN.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <strong>日本語</strong> · <a href="README.ko.md">한국어</a> · <a href="README.fr.md">Français</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.4.1400-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-無料-red">
</p>

---

## 概要

検索、翻訳、認識、プレビュー、一括処理、スクリーンショット、壁紙——日常的に頻繁に発生する細かな作業のために、拡張機能ストアで一つずつ探し、インストールし、課金する必要はもうありません。yyzTools を一度入れておけば、必要なものはいつでもすぐ手元にあります。

## 特長

- **永久無料** — 全機能を開放しています。有料アンロックや会員制度はなく、中核機能を制限して実質的に課金することもありません。
- **ローカル優先** — クリップボード、ファイル検索、使用統計、OCR などのデータは、すべてお使いのパソコン内で処理・保存され、自動的にアップロードされることはありません。
- **一度のインストールで完結** — 40 以上の機能モジュールがすぐに使えます。一つずつ探して、比べて、ダウンロードして、試して、削除するという手間が要りません。
- **379 個のコマンドモジュール** — AI、アプリ、ゲーム、システム、Web サイトの 5 分類のコマンドをすぐに利用できます。JSON ファイルを 1 つ追加するだけで自分で拡張できます。

## 機能一覧

| | |
|---|---|
| **コマンドパレット** | アプリ検索 · ファイル検索 · プロセス管理 · ウィンドウ管理 · ブックマーク検索 · クリップボード履歴 · コマンドライン · 即時計算 |
| **ランチャーと効率化** | ドック · クリップボード履歴 · アプリ使用統計 · スクリーンショット · 画面収録 · リマインダー · 万年暦 · キー入力表示 · マウス位置表示 |
| **テキストと言語** | スーパー翻訳 · 文字認識 OCR · 正規表現 · エンコード・デコード · ハッシュ計算 · 暗号化・復号 · データ変換 · データ生成 |
| **ファイルとメディア** | ファイルプレビュー · 一括リネーム · 画像の一括処理 · 動画の一括処理 · PDF の一括処理 · ファイルダウンロード |
| **計算と拡張** | 計算用紙 · QR コード · コマンドモジュール · 内蔵ブラウザー · カスタム拡張 |
| **パーソナライズ** | ライブ壁紙 · デスクトップエフェクト · ゲームモード · ナビゲーションページ · 地球防衛戦 |

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

- **Installer** (recommended): `yyzTools-setup-1.0.4.1400.exe` — [GitHub Releases](https://github.com/jearry/yyzTools/releases)
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

## License & Acknowledgments

yyzTools is a free software. The source code is not open source.

The software incorporates several open-source libraries and tools. See [LICENSE](LICENSE) for the full list of third-party components and their respective licenses.

We sincerely thank all the developers who have contributed to these open-source projects!
