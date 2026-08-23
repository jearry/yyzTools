<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.en.md">English</a> · <a href="README.zh-CN.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.ja.md">日本語</a> · <strong>한국어</strong> · <a href="README.fr.md">Français</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.3.1300-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-무료-red">
</p>

---

## 소개

검색, 번역, 인식, 미리보기, 일괄 처리, 화면 캡처, 배경화면까지 — 매일 자주 쓰지만 자잘한 작업들을 위해 더 이상 플러그인 마켓에서 하나씩 고르고 설치하고 결제할 필요가 없습니다. yyzTools를 한 번 설치하면 필요한 도구가 언제나 손 안에 있습니다.

## 핵심 특징

- **평생 무료** — 모든 기능을 개방합니다. 유료 잠금 해제도, 멤버십 등급도 없으며 핵심 기능을 제한해 우회적으로 과금하지도 않습니다.
- **로컬 우선** — 클립보드, 파일 검색, 사용 통계, OCR 등의 데이터는 모두 사용자 PC에서 처리하고 저장하며, 임의로 업로드하지 않습니다.
- **한 번의 설치** — 40여 개 기능 모듈을 설치 직후 바로 사용할 수 있어, 하나씩 검색하고 비교하고 다운로드하고 써 보고 지우는 과정이 필요 없습니다.
- **379개 명령 모듈** — AI, 앱, 게임, 시스템, 웹사이트 다섯 가지 분류의 명령을 바로 사용할 수 있고, JSON 파일 하나만 추가하면 직접 확장할 수 있습니다.

## 기능 개요

| | |
|---|---|
| **커맨드 팔레트** | 앱 검색 · 파일 검색 · 프로세스 관리 · 창 관리 · 북마크 검색 · 클립보드 기록 · 명령줄 · 즉시 계산 |
| **실행과 효율** | 독 · 클립보드 기록 · 앱 사용 통계 · 화면 캡처 · 화면 녹화 · 알림 도우미 · 만세력 · 키 입력 표시 · 마우스 찾기 |
| **텍스트와 언어** | 슈퍼 번역 · 문자 인식 OCR · 정규식 · 인코딩 · 디코딩 · 해시 계산 · 암호화 · 복호화 · 데이터 변환 · 데이터 생성 |
| **파일과 미디어** | 파일 미리보기 · 일괄 이름 변경 · 이미지 일괄 처리 · 비디오 일괄 처리 · PDF 일괄 처리 · 파일 다운로드 |
| **계산과 확장** | 계산 노트 · QR 코드 · 명령 모듈 · 내장 브라우저 · 사용자 확장 |
| **개인화** | 다이내믹 배경화면 · 바탕화면 특수 효과 · 게임 모드 · 내비게이션 페이지 · 지구 방위전 |

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

## License & Acknowledgments

yyzTools is a free software. The source code is not open source.

The software incorporates several open-source libraries and tools. See [LICENSE](LICENSE) for the full list of third-party components and their respective licenses.

We sincerely thank all the developers who have contributed to these open-source projects!
