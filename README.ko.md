<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.md">English</a> · <a href="README.zh-CN.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.ja.md">日本語</a> · <strong>한국어</strong> · <a href="README.fr.md">Français</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a> · <a href="README.pt.md">Português</a> · <a href="README.it.md">Italiano</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.5.1500-blue">
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

## 아키텍처

하이브리드 구조: **C++(Win32 + WebView2) 호스트**가 **프런트엔드(Alpine.js + 바닐라 JS + Vite)**를 담고 있으며, 기능 영역은 프로세스로 격리되어 있습니다.

| 프로세스 | 역할 |
|---------|------|
| `yyzTools.exe` | 메인 프로세스 — 커맨드 팔레트 / 번역 / OCR 등 대부분의 기능 탑재 |
| `yyzWallpaper.exe` | 다이내믹 배경화면 — Windows Composition API로 데스크톱 아이콘 레이어 아래에 합성 |
| `yyzBrowser.exe` | 명령 모듈이 URL을 열 때 사용하는 프레임 없는 내장 브라우저 |
| `yyzCmd.exe` | 순수 Win32 명령 실행기 (종료 / 재시작 / 볼륨 / 디스플레이 / 휴지통 등) |
| `yyzInputHint.exe` | 키보드 / 마우스 버튼 실시간 표시 (RawInput) |
| `yyzMouseFinder.exe` | 마우스 커서 빠르게 찾기 |
| `yyzUpdater.exe` | 증분 자동 업데이터 (`src/updater/` 참조) |
| `yyzFileSearch.exe` | 자체 개발 전체 볼륨 파일 검색 엔진 — MFT 직접 읽기 + USN 증분 인덱스 |

프런트엔드는 `window.Zen` 브리지를 통해 C++의 각 Manager를 호출하며, 모두 `{ error, ... }` 계약으로 통일된 결과를 반환합니다.

## 다운로드

- **설치 패키지**(권장): `yyzTools-setup-1.0.5.1500.exe` —— [GitHub Releases](https://github.com/jearry/yyzTools/releases)
- 중국 내부에서 접속이 느리면 미러를 이용하세요: [ghfast.top](https://ghfast.top/) · [ghproxy.com](https://ghproxy.com/) · [gh-proxy.com](https://gh-proxy.com/) (설치 패키지 다운로드 URL 앞에 해당 접두사를 붙이면 됩니다)
- [공식 다운로드 페이지](https://yyztools.com/ko/download.html)에서도 받을 수 있습니다.

설치 마법사는 중국어 간체·번체, 영어, 일본어, 한국어, 프랑스어, 독일어, 스페인어, 러시아어, 아랍어, 포르투갈어, 이탈리아어 12개 언어를 지원합니다.

## 시스템 요구 사항

- Windows 10 / 11 (x64)
- WebView2 Runtime (최신 시스템에는 미리 설치되어 있으며, 없으면 설치 패키지가 자동으로 설치합니다)

## 인터페이스 언어

인터페이스와 명령 모듈 문안 모두 위 12개 언어를 지원합니다. 첫 실행 시 시스템 언어를 자동으로 감지하며, 설정에서 언제든 변경할 수 있습니다.

## 이 저장소에 대하여

이 저장소(`jearry/yyzTools`)는 yyzTools의 **릴리스 및 공식 웹사이트 호스팅 저장소**입니다:

- [`releases/`](releases/) —— 버전 관리된 설치 패키지와 분할 압축 파일(`.7z`), 자동 업데이트 매니페스트 `update.json`
- [`docs/`](docs/) —— 공식 웹사이트 <https://yyztools.com>의 정적 사이트 (GitHub Pages로 호스팅)
- [`src/updater/`](src/updater/) —— 자동 업데이터 소스 코드

기능 관련 문제는 [Issues](https://github.com/jearry/yyzTools/issues)를 이용해 주세요.

## 라이선스 및 감사

yyzTools는 독점 소프트웨어이며 무료로 사용할 수 있습니다. 현재 자동 업데이트 모듈만 오픈소스로 공개되어 있으며, 해당 오픈소스 컴포넌트는 **MIT 라이선스**를 따릅니다. 향후 다른 부분 또는 전체를 오픈소스로 공개할지는 상황에 따라 결정됩니다.

본 소프트웨어는 여러 오픈소스 라이브러리와 도구를 사용합니다. 서드파티 컴포넌트의 전체 목록과 각 라이선스는 [LICENSE](LICENSE)를 참조하십시오.

이 오픈소스 프로젝트에 기여해 주신 모든 개발자분들께 진심으로 감사드립니다!
