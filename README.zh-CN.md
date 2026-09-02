<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.md">English</a> · <strong>简体中文</strong> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.fr.md">Français</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a> · <a href="README.pt.md">Português</a> · <a href="README.it.md">Italiano</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.5.1500-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-免费-red">
</p>

---

## 简介

搜索、翻译、识别、预览、批量处理、截图、壁纸——日常那些高频又琐碎的需求，不必再为每一个去插件市场里单独筛选、安装和付费。装一次 yyzTools，需要什么随手就有。

## 核心特色

- **永久免费** — 全部功能开放，不设付费解锁，不设会员体系，也不会用阉割核心能力的方式变相收费。
- **本地优先** — 剪贴板、文件搜索、使用统计、OCR 等数据都在你自己的电脑上处理和保存，不会被主动上传。
- **一次安装** — 40+ 功能模块开箱即用，省掉逐个搜索、比较、下载、试用、卸载的那一圈折腾。
- **379 个命令模块** — AI、应用、游戏、系统、网站五大类命令随取随用，加一个 JSON 文件就能自己扩展。

## 功能总览

| | |
|---|---|
| **命令面板** | 应用搜索 · 文件搜索 · 进程管理 · 窗口管理 · 书签搜索 · 剪贴板历史 · 命令行 · 即时计算 |
| **启动与效率** | 程序坞 · 剪贴板历史 · 应用统计 · 屏幕截图 · 屏幕录制 · 提醒助手 · 万年历 · 按键提示 · 鼠标定位 |
| **文本与语言** | 超级翻译 · 文字识别 OCR · 正则表达式 · 编码解码 · 哈希计算 · 加密解密 · 数据转换 · 数据生成 |
| **文件与媒体** | 文件预览 · 批量重命名 · 图片批量处理 · 视频批量处理 · PDF 批量处理 · 文件下载 |
| **计算与拓展** | 计算稿纸 · 二维码 · 命令模块 · 内置浏览器 · 自定义扩展 |
| **个性化** | 动态壁纸 · 桌面特效 · 游戏模式 · 导航页 · 地球保卫战 |

## 架构

混合架构：**C++（Win32 + WebView2）宿主** 承载 **前端（Alpine.js + 原生 JS + Vite）**，多进程隔离。

| 进程 | 职责 |
|------|------|
| `yyzTools.exe` | 主进程，承载命令面板 / 翻译 / OCR 等绝大多数功能 |
| `yyzWallpaper.exe` | 动态壁纸，用 Windows Composition API 贴到桌面图标层下方 |
| `yyzBrowser.exe` | 内置无边框浏览器，供命令模块打开网址 |
| `yyzCmd.exe` | 纯 Win32 命令执行器（关机 / 重启 / 音量 / 显示器 / 回收站等） |
| `yyzInputHint.exe` | 键盘 / 鼠标按键实时提示（RawInput） |
| `yyzMouseFinder.exe` | 快速定位鼠标光标 |
| `yyzUpdater.exe` | 增量自动更新器（见 `src/updater/`） |

前端通过 `window.Zen` 桥接调用 C++ 各 Manager，统一返回 `{ error, ... }` 契约。

## 下载安装

- **安装包**（推荐）：`yyzTools-setup-1.0.5.1500.exe` —— [GitHub Releases](https://github.com/jearry/yyzTools/releases)
- 国内访问慢可走镜像：[ghfast.top](https://ghfast.top/) · [ghproxy.com](https://ghproxy.com/) · [gh-proxy.com](https://gh-proxy.com/)（在安装包下载链接前加对应前缀）
- 也可从 [官网下载页](https://yyztools.com/zh-CN/download.html) 获取。

安装向导支持简体中文、繁体中文、英语、日语、韩语、法语、德语、西班牙语、俄语、阿拉伯语、葡萄牙语、意大利语 12 种语言。

## 系统要求

- Windows 10 / 11（x64）
- WebView2 Runtime（较新系统已自带，缺失时安装包会自动补装）

## 界面语言

界面与命令模块文案均支持上述 12 种语言；首次启动按系统语言自动匹配，可在设置中切换。

## 本仓库说明

本仓库（`jearry/yyzTools`）是 yyzTools 的 **发布与官网托管仓库**：

- [`releases/`](releases/) —— 版本化安装包与分卷压缩包（`.7z`），以及自动更新清单 `update.json`
- [`docs/`](docs/) —— 官网 <https://yyztools.com> 的静态站点（经 GitHub Pages 托管）
- [`src/updater/`](src/updater/) —— 自动更新器源码

如需反馈功能问题，请至 [Issues](https://github.com/jearry/yyzTools/issues)。

## 许可证与致谢

yyzTools 是专有软件，可免费使用。目前仅开源了自动更新部分，该开源组件遵循 **MIT 协议**。后续是否开源其他部分和全部开源，将根据情况决定。

本软件使用了多个开源库和工具，完整的第三方组件列表及其许可证请参见 [LICENSE](LICENSE)。

衷心感谢所有为这些开源项目做出贡献的开发者们！
