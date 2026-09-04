<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.md">English</a> · <a href="README.zh-CN.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.fr.md">Français</a> · <strong>Deutsch</strong> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a> · <a href="README.pt.md">Português</a> · <a href="README.it.md">Italiano</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.6.1600-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-kostenlos-red">
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

## Architektur

Hybrid: Ein **C++-Host (Win32 + WebView2)** trägt ein **Frontend (Alpine.js + Vanilla-JS + Vite)**, mit nach Prozessen getrennten Funktionsbereichen.

| Prozess | Aufgabe |
|---------|---------|
| `yyzTools.exe` | Hauptprozess — beherbergt die Kommandoleiste, Übersetzung, OCR und die meisten weiteren Funktionen |
| `yyzWallpaper.exe` | Dynamischer Hintergrund — blendet Web-/Videoinhalte über die Windows Composition API unter die Desktop-Symbolebene |
| `yyzBrowser.exe` | Rahmenloser integrierter Browser, über den Kommandomodule URLs öffnen |
| `yyzCmd.exe` | Reiner Win32-Befehlsausführer (Herunterfahren / Neustart / Lautstärke / Anzeige / Papierkorb usw.) |
| `yyzInputHint.exe` | Echtzeit-Anzeige von Tastatur- / Maustasten (RawInput) |
| `yyzMouseFinder.exe` | Mauszeiger schnell lokalisieren |
| `yyzUpdater.exe` | Inkrementeller Auto-Updater (siehe `src/updater/`) |
| `yyzFileSearch.exe` | Eigenentwickelte Vollvolume-Dateisuche — direktes MFT-Lesen + inkrementeller USN-Index |

Das Frontend ruft die C++-Manager über die `window.Zen`-Brücke auf; alle antworten nach dem einheitlichen `{ error, ... }`-Vertrag.

## Download

- **Installer** (empfohlen): `yyzTools-setup-1.0.6.1600.exe` —— [GitHub Releases](https://github.com/jearry/yyzTools/releases)
- Aus China langsam? Stellen Sie einem Mirror den Installer-URL voran: [ghfast.top](https://ghfast.top/) · [ghproxy.com](https://ghproxy.com/) · [gh-proxy.com](https://gh-proxy.com/)
- Alternativ über die [offizielle Download-Seite](https://yyztools.com/de/download.html).

Der Installationsassistent unterstützt 12 Sprachen: Chinesisch (vereinfacht und traditionell), Englisch, Japanisch, Koreanisch, Französisch, Deutsch, Spanisch, Russisch, Arabisch, Portugiesisch und Italienisch.

## Systemvoraussetzungen

- Windows 10 / 11 (x64)
- WebView2 Runtime (auf neueren Systemen vorinstalliert; der Installer zieht sie bei Bedarf nach)

## Sprachen der Oberfläche

Sowohl die Oberfläche als auch die Texte der Kommandomodule unterstützen die oben genannten 12 Sprachen. Beim ersten Start wird die Systemsprache automatisch erkannt; ein Wechsel ist jederzeit in den Einstellungen möglich.

## Über dieses Repository

Dieses Repository (`jearry/yyzTools`) ist das **Release- und Hosting-Repository** von yyzTools für Veröffentlichungen und die offizielle Website:

- [`releases/`](releases/) —— versionierte Installer und Split-Archive (`.7z`) sowie das Auto-Update-Manifest `update.json`
- [`docs/`](docs/) —— statische Website von <https://yyztools.com> (über GitHub Pages gehostet)
- [`src/updater/`](src/updater/) —— Quellcode des Auto-Updaters

Für Problemmeldungen zu Funktionen nutzen Sie bitte [Issues](https://github.com/jearry/yyzTools/issues).

## Lizenz und Danksagung

yyzTools ist proprietäre Software und kostenlos nutzbar. Derzeit ist nur das automatische Update-Modul als Open Source veröffentlicht, unter der **MIT-Lizenz**. Ob weitere Teile oder die gesamte Anwendung open-sourced werden, wird je nach Situation entschieden.

Diese Software verwendet mehrere Open-Source-Bibliotheken und -Werkzeuge. Die vollständige Liste der Drittkomponenten und deren Lizenzen finden Sie in [LICENSE](LICENSE).

Wir danken aufrichtig allen Entwicklern, die zu diesen Open-Source-Projekten beigetragen haben!
