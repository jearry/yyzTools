<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.md">English</a> · <a href="README.zh-CN.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.fr.md">Français</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a> · <a href="README.pt.md">Português</a> · <strong>Italiano</strong></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.6.1650-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-Gratuito-red">
</p>

---
## Screenshot

<p align="center">
  <img src="screenshots/en/01-cmd-plate-home.png" alt="Palette dei comandi">
  <img src="screenshots/en/05-dock-overview.png" alt="Dock">
</p>

---

## Introduzione

Cercare, tradurre, riconoscere testi, visualizzare in anteprima, elaborare in batch, fare schermate, sfondi animati — le piccole attività frequenti del lavoro quotidiano, senza dover cercare, installare e pagare uno strumento separato per ciascuna. Installa yyzTools una volta e ciò che ti serve è già a portata di mano.

## Punti di forza

- **Gratuito per sempre** — Tutte le funzionalità sono aperte: nessuno sblocco a pagamento, nessun sistema a membri e nessun addebito mascherato tramite limitazioni delle capacità fondamentali.
- **Locale prima di tutto** — Appunti, ricerca file, statistiche di utilizzo ed OCR vengono elaborati e salvati sul tuo computer, senza invii automatici.
- **Una sola installazione** — Oltre 40 moduli pronti all'uso, risparmiando l'intero ciclo di ricerca, confronto, download, prova e disinstallazione.
- **379 moduli di comandi** — Comandi in cinque categorie (IA, app, giochi, sistema e siti) sempre disponibili; basta un file JSON per aggiungere i tuoi.

## Panoramica delle Funzionalità

| | |
|---|---|
| **Pannello dei comandi** | Ricerca app · Ricerca file · Gestione processi · Gestione finestre · Ricerca segnalibri · Cronologia appunti · Riga di comando · Calcolo immediato |
| **Avvio e produttività** | Dock · Cronologia appunti · Statistiche app · Schermata · Registrazione schermo · Assistente promemoria · Calendario perpetuo · Suggerimento tasti · Individuazione mouse |
| **Testo e lingue** | Super traduzione · Riconoscimento testi OCR · Espressioni regolari · Codifica/decodifica · Calcolo hash · Crittografia/decrittografia · Conversione dati · Generazione dati |
| **File e multimedialità** | Anteprima file · Rinomina in batch · Elaborazione immagini in batch · Elaborazione video in batch · Elaborazione PDF in batch · Download file |
| **Calcolo ed estensioni** | Foglio di calcolo · Codice QR · Moduli di comandi · Browser integrato · Estensioni personalizzate |
| **Personalizzazione** | Sfondi animati · Effetti desktop · Modalità gioco · Pagina di navigazione · Difesa della Terra |

## Architettura

Architettura ibrida: un **host C++ (Win32 + WebView2)** che ospita il **frontend (Alpine.js + JS nativo + Vite)**, con isolamento tra processi.

| Processo | Responsabilità |
|----------|----------------|
| `yyzTools.exe` | Processo principale — ospita il pannello dei comandi, la traduzione, l'OCR e la maggior parte delle funzionalità |
| `yyzWallpaper.exe` | Sfondo animato, applicato sotto il livello delle icone del desktop tramite la Windows Composition API |
| `yyzBrowser.exe` | Browser integrato senza bordi, usato dai moduli di comandi per aprire URL |
| `yyzCmd.exe` | Esecutore di comandi in Win32 puro (spegnimento / riavvio / volume / monitor / cestino, ecc.) |
| `yyzInputHint.exe` | Visualizzazione in tempo reale dei tasti premuti (RawInput) |
| `yyzMouseFinder.exe` | Individuazione rapida del cursore del mouse |
| `yyzUpdater.exe` | Aggiornatore automatico incrementale (vedi `src/updater/`) |
| `yyzFileSearch.exe` | Motore di ricerca file interno — lettura diretta della MFT + indice incrementale USN |

Il frontend richiama i vari Manager del C++ tramite il bridge `window.Zen`, restituendo sempre il contratto unificato `{ error, ... }`.

## Download e Installazione

- **Installazione** (consigliata): `yyzTools-setup-1.0.6.1650.exe` —— [GitHub Releases](https://github.com/jearry/yyzTools/releases)
- Se l'accesso dalla Cina è lento, usa un mirror: [ghfast.top](https://ghfast.top/) · [ghproxy.com](https://ghproxy.com/) · [gh-proxy.com](https://gh-proxy.com/) (aggiungi il prefisso corrispondente prima del link di download)
- Disponibile anche dalla [pagina di download ufficiale](https://yyztools.com/it/download.html).

L'installazione guidata supporta 12 lingue: cinese semplificato e tradizionale, inglese, giapponese, coreano, francese, tedesco, spagnolo, russo, arabo, portoghese e italiano.

## Requisiti di Sistema

- Windows 10 / 11 (x64)
- WebView2 Runtime (già incluso nei sistemi recenti; l'installer lo installa automaticamente se manca)

## Lingua dell'Interfaccia

L'interfaccia e i testi dei moduli di comandi supportano le 12 lingue sopra indicate; al primo avvio la lingua viene abbinata automaticamente a quella di sistema e può essere cambiata nelle impostazioni.

## Informazioni su Questo Repository

Questo repository (`jearry/yyzTools`) è il **repository di pubblicazione e di hosting del sito ufficiale** di yyzTools:

- [`releases/`](releases/) —— Installer versionati e archivi divisi (`.7z`), oltre al manifest di aggiornamento automatico `update.json`
- [`docs/`](docs/) —— Sito statico di <https://yyztools.com> (ospitato su GitHub Pages)
- [`src/updater/`](src/updater/) —— Codice sorgente dell'aggiornatore automatico

Per segnalare problemi di funzionalità, utilizza [Issues](https://github.com/jearry/yyzTools/issues).

## Licenza e Ringraziamenti

yyzTools è software proprietario, utilizzabile gratuitamente. Attualmente solo la parte di aggiornamento automatico è open source, concessa sotto la **Licenza MIT**. La decisione di aprire il codice di altre parti o dell'intera applicazione verrà presa in base alle circostanze.

Questo software utilizza diverse librerie e strumenti open source; l'elenco completo dei componenti di terze parti e delle relative licenze è disponibile in [LICENSE](LICENSE).

Un sincero ringraziamento a tutti gli sviluppatori che contribuiscono a questi progetti open source!
