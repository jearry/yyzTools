<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.en.md">English</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.fr.md">Français</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <strong>العربية</strong></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.2.1200-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-MIT-success">
</p>

---

## مقدمة

البحث والترجمة والتعرّف على النصوص والمعاينة والمعالجة الدفعية والتقاط الشاشة والخلفيات — كل تلك المهام اليومية المتكررة والصغيرة، دون أن تبحث في متاجر الإضافات عن أداة منفصلة لكل واحدة منها وتثبّتها وتدفع ثمنها. ثبّت yyzTools مرة واحدة، وستجد ما تحتاجه في متناول يدك.

## أبرز المزايا

- **مجاني للأبد** — جميع المزايا متاحة بالكامل: لا فتح مدفوع، ولا نظام عضويات، ولا رسوم مقنّعة عبر تقييد القدرات الأساسية.
- **المعالجة محليًا أولًا** — بيانات الحافظة والبحث في الملفات وإحصاءات الاستخدام والتعرّف على النصوص تُعالَج وتُخزَّن على جهازك، ولا تُرفَع من تلقاء نفسها.
- **تثبيت واحد** — أكثر من 40 وحدة جاهزة للعمل فورًا، فتوفّر على نفسك دورة البحث والمقارنة والتنزيل والتجربة وإلغاء التثبيت.
- **379 وحدة أوامر** — أوامر جاهزة في خمس فئات: الذكاء الاصطناعي والتطبيقات والألعاب والنظام والمواقع، ويكفي ملف JSON واحد لإضافة أوامرك الخاصة.

## الميزات

| | |
|---|---|
| **لوحة الأوامر** | البحث في التطبيقات · البحث في الملفات · إدارة العمليات · إدارة النوافذ · البحث في الإشارات المرجعية · سجل الحافظة · سطر الأوامر · الحساب الفوري |
| **التشغيل والإنتاجية** | الرصيف · سجل الحافظة · إحصاءات التطبيقات · التقاط الشاشة · مساعد التذكيرات · التقويم الدائم · تلميحات المفاتيح · تحديد موقع الفأرة |
| **النص واللغة** | الترجمة الفائقة · التعرّف على النصوص OCR · التعابير النمطية · الترميز وفك الترميز · حساب التجزئة · التشفير وفك التشفير · تحويل البيانات · توليد البيانات |
| **الملفات والوسائط** | معاينة الملفات · إعادة التسمية دفعةً · المعالجة الدفعية للصور · المعالجة الدفعية للفيديو · المعالجة الدفعية لملفات PDF · تنزيل الملفات |
| **الحساب والإضافات** | مسودة الحساب · رمز QR · وحدات الأوامر · المتصفح المدمج · الإضافات المخصّصة |
| **التخصيص** | الخلفيات الديناميكية · مؤثرات سطح المكتب · وضع اللعب · صفحة التنقل · الدفاع عن الأرض |

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
