<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.md">English</a> · <a href="README.zh-CN.md">简体中文</a> · <strong>繁體中文</strong> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.fr.md">Français</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a> · <a href="README.pt.md">Português</a> · <a href="README.it.md">Italiano</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.6.1650-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-免費-red">
</p>

---

## 簡介

搜尋、翻譯、辨識、預覽、批次處理、截圖、桌布——日常那些高頻又瑣碎的需求，不必再為每一個去外掛市集裡單獨篩選、安裝與付費。裝一次 yyzTools，需要什麼隨手就有。

## 核心特色

- **永久免費** — 全部功能開放，不設付費解鎖，不設會員制，也不會用刻意削減核心能力的方式變相收費。
- **本機優先** — 剪貼簿、檔案搜尋、使用統計、OCR 等資料都在你自己的電腦上處理與儲存，不會被主動上傳。
- **一次安裝** — 40+ 功能模組開箱即用，省掉逐個搜尋、比較、下載、試用、移除的那一圈折騰。
- **379 個命令模組** — AI、應用程式、遊戲、系統、網站五大類命令隨取隨用，加一個 JSON 檔案就能自己擴充。

## 功能總覽

| | |
|---|---|
| **命令面板** | 應用程式搜尋 · 檔案搜尋 · 處理程序管理 · 視窗管理 · 書籤搜尋 · 剪貼簿歷史 · 命令列 · 即時計算 |
| **啟動與效率** | 程式啟動列 · 剪貼簿歷史 · 應用程式統計 · 螢幕截圖 · 螢幕錄製 · 提醒助理 · 萬年曆 · 按鍵提示 · 滑鼠定位 |
| **文字與語言** | 超級翻譯 · 文字辨識 OCR · 正規表示式 · 編碼解碼 · 雜湊計算 · 加密解密 · 資料轉換 · 資料產生 |
| **檔案與媒體** | 檔案預覽 · 批次重新命名 · 圖片批次處理 · 影片批次處理 · PDF 批次處理 · 檔案下載 |
| **計算與擴充** | 計算稿紙 · QR Code · 命令模組 · 內建瀏覽器 · 自訂擴充 |
| **個人化** | 動態桌布 · 桌面特效 · 遊戲模式 · 導覽頁 · 地球保衛戰 |

## 架構

混合架構：**C++（Win32 + WebView2）宿主** 承載 **前端（Alpine.js + 原生 JS + Vite）**，多程序隔離。

| 程序 | 職責 |
|------|------|
| `yyzTools.exe` | 主程序，承載命令面板 / 翻譯 / OCR 等絕大多數功能 |
| `yyzWallpaper.exe` | 動態桌布，用 Windows Composition API 貼到桌面圖示層下方 |
| `yyzBrowser.exe` | 內建無邊框瀏覽器，供命令模組開啟網址 |
| `yyzCmd.exe` | 純 Win32 命令執行器（關機 / 重啟 / 音量 / 顯示器 / 回收筒等） |
| `yyzInputHint.exe` | 鍵盤 / 滑鼠按鍵即時提示（RawInput） |
| `yyzMouseFinder.exe` | 快速定位滑鼠游標 |
| `yyzUpdater.exe` | 增量自動更新器（見 `src/updater/`） |
| `yyzFileSearch.exe` | 自研全碟檔案搜尋引擎——MFT 直讀 + USN 增量索引 |

前端透過 `window.Zen` 橋接呼叫 C++ 各 Manager，統一回傳 `{ error, ... }` 契約。

## 下載安裝

- **安裝包**（推薦）：`yyzTools-setup-1.0.6.1650.exe` —— [GitHub Releases](https://github.com/jearry/yyzTools/releases)
- 國內存取慢可走鏡像：[ghfast.top](https://ghfast.top/) · [ghproxy.com](https://ghproxy.com/) · [gh-proxy.com](https://gh-proxy.com/)（在安裝包下載連結前加對應前綴）
- 也可從[官網下載頁](https://yyztools.com/zh-TW/download.html)取得。

安裝精靈支援簡體中文、繁體中文、英語、日語、韓語、法語、德語、西班牙語、俄語、阿拉伯語、葡萄牙語、義大利語 12 種語言。

## 系統需求

- Windows 10 / 11（x64）
- WebView2 Runtime（較新系統已內建，缺失時安裝包會自動補裝）

## 介面語言

介面與命令模組文案均支援上述 12 種語言；首次啟動依系統語言自動匹配，可在設定中切換。

## 本倉庫說明

本倉庫（`jearry/yyzTools`）是 yyzTools 的 **發布與官網託管倉庫**：

- [`releases/`](releases/) —— 版本化安裝包與分卷壓縮包（`.7z`），以及自動更新清單 `update.json`
- [`docs/`](docs/) —— 官網 <https://yyztools.com> 的靜態網站（經 GitHub Pages 託管）
- [`src/updater/`](src/updater/) —— 自動更新器原始碼

如需回報功能問題，請至 [Issues](https://github.com/jearry/yyzTools/issues)。

## 授權條款與致謝

yyzTools 是專有軟體，可免費使用。目前僅開源了自動更新部分，該開源元件遵循 **MIT 協議**。後續是否開源其他部分和全部開源，將根據情況決定。

本軟體使用了多個開源庫與工具，完整的第三方元件清單及其許可證請參見 [LICENSE](LICENSE)。

衷心感謝所有為這些開源專案做出貢獻的開發者們！
