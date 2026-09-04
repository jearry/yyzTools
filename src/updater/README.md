# yyzUpdater

yyzTools 的自动更新器：拉远端 `update.json` 逐包比版本 → aria2 多源下载 → sha256 校验 → 7z 解压 → 替换文件 → 重启主程序。由主进程 `yyzTools.exe` 用命令行调起（`Application.cpp` 的定时静默检查、`Application::PreRun` 的待应用检测、`NativeApi::CheckUpdate` / `NativeApi::ApplyUpdate` 的托盘手动触发）。

Windows 子系统程序，**全程无 UI**（无窗口、无 MessageBox、无进度条），进度只写日志；单实例（命名互斥体 `Local\yyzUpdater_SingleInstance`），干完即退出。退出码 `0` 成功、`1` 失败。

## 用法

```
yyzUpdater.exe [--check | --apply | --check-and-apply]
```

`Main.cpp` 把 `argv[1..]` 拼成一个字符串，用 `std::wstring::find` **子串匹配**判定，判定顺序即优先级：

| 参数 | 入口 | 行为 |
|------|------|------|
| `--check-and-apply` | `RunCheckAndApply` | 先清 `update.result`；已有 `update.ready` 则直接 apply；否则检查，有更新即 apply，无更新/失败写 `update.result` 标记 |
| `--apply` | `RunApply` | 读 `update.ready` → 杀进程 → 解压 → 替换 → 重启 `yyzTools.exe` |
| 其它 / 无参 | `RunCheck` | 默认分支：检查 + 下载 + 校验 + 写 `update.ready` 后退出，不动任何已安装文件 |

不支持 `--key=value` 形式，也没有其它参数。

## 目录布局

安装目录即更新目标：`%APPDATA%\yyzTools`（`inno_setup/yyztools.iss` 的 `DefaultDirName={userappdata}\{#SoftIdent}`，`PrivilegesRequired=lowest`，故整个更新流程默认不需要提权）。

| 路径 | 取值函数 | 内容 |
|------|----------|------|
| `%APPDATA%\yyzTools` | `GetInstallDir()` | 替换目标根；同时是 `Platform\Aria2\aria2c.exe`、`Platform\7z\7z.exe` 的定位基准 |
| `…\Update` | `GetUpdateDir()` | `client_versions.json`、`client_update.json`、`update.json`、`update.ready`、`update.result` |
| `…\Update\downloads` | `GetDownloadDir()` | `<包名>-<版本>.7z` 持久缓存（跨进程保留，配合 sha256 复用） |
| `…\Update\staging\<包名>` | `GetStagingDir()` | 解压暂存，每包一子目录，应用完即删 |
| `…\Update\tools` | `GetToolsDir()` | `7z.exe` / `7z.dll` 运行副本 |
| `%APPDATA%\yyzTools\logs\updater.log` | `..\public\logger.cpp` | 日志 |

`EnsureDir`（`updater.cpp`）是 `EnsureDirTree`（`applying.cpp`，逐级创建）的取路径返回值包装，故 `Update` 及其子目录即使父目录缺失也能一次建出。`GetUpdateDir` 走 `GetAppDataRoot() + "\\Update"` 而**不经 `GetInstallDir`**，两者互不依赖。

## 更新检查（`DoCheck`）

- **update.json 源**：`client_update.json` 的 `updateJsonBases`，每个 base 拼 `/releases/update.json` 组成多源列表。配置文件不存在时用 `FALLBACK_UPDATE_JSON_BASE`（`https://raw.githubusercontent.com/jearry/yyzTools/main`）兜底，并把这份默认写回文件供用户编辑。
- **本地版本**：只读 `Update\client_versions.json`（`包名: 版本` 的纯字典），**不读 exe 的版本资源**。该文件由安装包铺下，之后由 updater 自己 `SaveVersions` 覆写。
- **比较**：`CompareVersion`（`version.h`）按 `.` 分段做数值比较，短的一侧零填充；`remote <= local` 即跳过该包。
- **update.json 结构**：顶层 `packages` 是对象，键为包名，每包 `version` / `url` / `sha256` / `killProcesses` / `killProcessesAdmin` / `stopService`；顶层还可有一个与更新无关的 `home` 块（见下）。
- **url 拼接**：`url` 为字符串时只放后缀 `v<该包版本号>/<文件名>`，updater 拼上 `RELEASE_BASE`（`https://github.com/jearry/yyzTools/releases/download`）；若后缀本身以 `http://` / `https://` 开头则原样当完整地址。`url` 也兼容数组形式，数组元素被原样当多源列表。
- **镜像**：`BuildMultiSourceUrls` 把 `releaseMirrors` 每一项作为前缀拼 `<mirror>/<realUrl>`，真实地址永远排第一。
- **排序**：命中的包按 `APPLY_ORDER` 排序后写 `update.ready`：`web` → `modules` → `wallpaper` → `yyztools` → `tools` → `yyzfilesearch` → `rapidocr` → `ffmpeg` → `main`（资源类先，`main` 含自更新故最后）。
- **结果**：全部无更新 → `kUpToDate`；写出 `update.ready` → `kReady`；任一包下载/校验失败 → `kError`。
- `update.json` 原文缓存在 `Update\update.json`，主进程 `NativeApi::GetHomeAd`（`Zen.getHomeAd`）读其 `home` 块作为命令面板主页广告位配置 —— 这个文件不是纯临时文件。

## 下载（`http_client.cpp`）

全部走 `Platform\Aria2\aria2c.exe` 子进程（`RunAria2`，`CREATE_NO_WINDOW` + `SW_HIDE`），固定参数：

```
--no-conf --console-log-level=warn --summary-interval=0
--allow-overwrite=true --auto-file-renaming=false
--split=16 --max-connection-per-server=16 --min-split-size=1M
--max-tries=3 --retry-wait=2 --timeout=60 --connect-timeout=30
--dir=<目录> --out=<文件名> <url1> <url2> ...
```

- 多个 url **一次性交给 aria2**，语义是「同一文件多源并行拉取合并」，不是逐个 fallback 重试。
- 超时：`DownloadText`（update.json）30 秒、`DownloadFile`（包）300 秒；超时即 `TerminateProcess`，返回非 0，本轮放弃留待下次。
- **不预删已存在文件**：靠 aria2 的 `.aria2` 控制文件断点续传，失败的部分文件刻意保留（日志 `partial kept for resume`）。
- `DownloadFile` 的 `progress` 形参保留但未接线（headless，不解析 aria2 输出）。
- `DownloadText` 直接让 aria2 `--out=update.json` 落到 `Update` 根，返回值再由 `DoCheck` 读文件内容。

## 校验（`hasher.cpp`）

- `Sha256File`：CNG `BCryptOpenAlgorithmProvider(BCRYPT_SHA256_ALGORITHM)` 流式哈希，64KB 缓冲，输出小写 hex；链接 `bcrypt.lib`。
- 与 `update.json` 的 `sha256` 用 `_stricmp` 比较（大小写不敏感）。
- 下载前若 `downloads` 里已有同名包，先算一次 sha256，命中即 `reuse cached package, skip download`。
- 校验不通过：**文件不删**，标记失败中断本轮，下次由 aria2 覆盖重下。

## 孤儿下载包清理（`CleanStaleDownloads`）

包文件名带版本号（`<包名>-<版本>.7z`），而 apply 成功时的 `DeleteFileW` 只认 `p.zipPath` 这一个精确路径。于是**远端版本一跳，上一轮没应用成功的包就成了永久孤儿** —— 1.0.4 升级失败后服务端直接发 1.0.5，`downloads` 里的 `*-1.0.4.*.7z` 不再被任何计划引用，1.0.5 应用成功也只删自己那几个。`.7z.aria2` 续传控制文件同理（它由 aria2 在下载完成时自行删除，中断的就一直留着）。

故 `DoCheck` 定版后按白名单收尾：扫 `downloads`，保留本轮 pending 各包的 `<文件名>` 与 `<文件名>.aria2`，其余文件全删。

- **无更新（`kUpToDate`）时白名单为空**，整个目录清空 —— 那种状态下任何残留都是孤儿。
- ⚠️ **失败路径（`failed`）刻意不清理**：`DoCheck` 遇到下载/校验失败是 `break`，`pending` 只覆盖到失败包为止，后面的包压根没检查过。拿这份不完整的列表当白名单会误删本该复用的缓存。留待下一轮检查成功时统一收尾。
- 删除失败（如文件被占用）只记日志不中断，下轮再试。

## 解压（`zip_extract.cpp`）

`ExtractArchive` 调 `Platform\7z\7z.exe`：

```
7z.exe x "<archive>" -o"<destDir>" -y -bd
```

`CREATE_NO_WINDOW` + `SW_HIDE`，超时 180 秒后 `TerminateProcess`；退出码非 0 视为失败。运行前把 `Platform\7z\7z.exe` 与 `7z.dll` `CopyFileW` 到 `Update\tools` 再从副本启动（原因见「约定与坑」）。`RunApply` 会在解压前 `RemoveDirRecursive(staging)` 保证目录干净。

## 应用更新（`applying.cpp` / `RunApply`）

时序：

1. `KillChildProcesses()` —— 当前只杀 `yyzTools.exe`（其余子进程改由各包 `killProcesses` 声明、按包杀）。
2. 逐包（已按 `APPLY_ORDER` 排序）：`KillPackageProcesses(killProcesses, killProcessesAdmin)` → 停 `stopService` 声明的服务 → 清 staging → `ExtractArchive` → `ApplyStaging` → 删 staging。
3. 该包 `ApplyStaging` 返回真（零失败）才 `UpdateLocalVersion` 写回 `client_versions.json` 并删掉 `.7z`；否则版本不升、包保留待下次重试。
4. 循环结束**先** `DeleteFileW(update.ready)`，**再** `LaunchMain(appDir)` 起 `%APPDATA%\yyzTools\yyzTools.exe`（工作目录为 appDir，不带参数）。

`StopServiceByName` / `StartServiceByName`：`stopService` 列出该包应用前要停的服务名（如 `yyzFileSearchSvc`）。普通权限 SCM 直接停；权限不足（服务跑 LocalSystem）才经 UAC runas 调 `sc.exe stop/start`。实际停掉的服务记入集合，全部包应用完统一 `StartServiceByName` 重启。

`KillPackageProcesses`：`killProcesses` 按映像名 `TerminateProcess`（`FindProcessIds` 返回同名的**全部** PID，多实例一并杀）；`killProcessesAdmin` 先普通杀，仍能找到才 `ShellExecuteExW("runas", "taskkill.exe", "/f /im <exe>")` 并等 5 秒 —— 先探活是为了避免进程本就不在时白弹 UAC。有杀动作才 `Sleep(500)`。

`ApplyStaging`：`CollectFiles` 递归收集 staging 内全部文件（连相对路径），逐文件：

- 目标目录不存在 → `EnsureDirTree` 逐级创建（`CreateDirectoryW` 只能建一层，包内新增多层目录靠它兜住）。
- 目标已存在 → 先 `MoveFileW` 成 `<dest>.bak`（自身 exe 用 `.old`），移动前先删同名残留。
- `MoveFileW` 新文件到目标；失败降级 `CopyFileW` + `DeleteFileW`（日志 `move failed, copy fallback ok`）。
- 成功且非自身 → `.bak` 入待删列表，全部文件处理完统一删；失败 → `MoveFileW` 把 `.bak` 挪回原位（**逐文件回滚，不是整包回滚**），`failures++`。
- 返回 `failures == 0`。

**自更新**：`main` 包含 `yyzTools.exe` + `yyzUpdater.exe` + `WebView2Loader.dll`，`APPLY_ORDER` 把它排在最后。`yyzUpdater.exe` 是当前运行进程的映像 —— Windows 允许改名运行中的 exe 但不允许删除/覆盖，故改名成 `yyzUpdater.exe.old` 让位，新版本 move 进来；`.old` 不入待删列表（此刻删不掉），由下次启动时 `Main.cpp` 开头的 `DeleteFileW` 清理。

## 日志（`..\public\logger.cpp`，公共模块）

`wWinMain` 第一件事就是 `LogInit(%APPDATA%\yyzTools\logs, updater.log)`（早于单例判定，所以「已有实例在跑」也留痕）。`Log` / `LogFmt`（`vsnprintf`，单条上限 2048 字节）以 `[YYYY-MM-DD HH:MM:SS] ` 前缀追加写 `logs\updater.log`，每条独立 open/append/close。超过 `kMaxLogSize`（10MB）时 `MoveFileExW` 轮转为 `updater.log.1`（覆盖旧备份，只保一份）。

## 与主进程的对接

| 触发点 | 代码位置 | 命令 |
|--------|----------|------|
| 启动后延迟 15 秒的静默检查，24 小时一次 | `src/yyztools/Application.cpp`（`CONF_AUTO_UPDATE_ENABLED` / `CONF_LAST_UPDATE_CHECK`，`DelayedTask`） | `--check` |
| 启动时发现 `Update\update.ready` | `Application::PreRun`，拉起后 `return -3` 让主进程直接退出让位 | `--apply` |
| 托盘「检查更新」 | `NativeApi::CheckUpdate`（`Zen.checkUpdate`） | `--check-and-apply` |
| 托盘「立即更新」（ready 态） | `NativeApi::ApplyUpdate`（`Zen.applyUpdate`） | `--apply` |

状态回读走 `NativeApi::GetUpdateState`（`Zen.getUpdateState`，`src/web/tray/tray.js` 每 2 秒轮询）：`OpenMutexW` 探到 `Local\yyzUpdater_SingleInstance` → `checking`；`update.ready` 存在 → `ready`；否则 `idle`，并读一次即删 `update.result`（内容 `uptodate` / `error`）。`update.result` 只在 `--check-and-apply` 路径写 —— 静默 `--check` 不写，否则用户下次打开托盘会莫名弹「已是最新版本」。

## 发布侧对接

单点真源是仓库根的 `versions.json`：

- `release` 是**发布身份号**：GitHub release tag `v<release>`、安装包文件名、`inno_setup/version.iss` 都由它派生，每次发版必递增。
- 9 个子包 `web` / `modules` / `wallpaper` / `yyztools` / `tools` / `yyzfilesearch` / `rapidocr` / `ffmpeg` / `main` 各自的版本号代表该包内容版本，没改就不升。`main` 只是主程序子包的版本，不是发布号。
- `packageKillProcesses.<包名>.killProcesses|killProcessesAdmin|stopService` → 进 `update.json` 同名字段 → 进 `update.ready` → `KillPackageProcesses` / `StopServiceByName` 消费。
- `updateJsonBases`（9 个源：jsDelivr 四镜像 / raw.githubusercontent / raw.gitmirror / ghfast / ghproxy / gh-proxy）与 `releaseMirrors`（`ghfast.top` / `ghproxy.com` / `gh-proxy.com`）→ `gen_json.ps1` 抽出写成 `inno_setup/client_update.json` → iss 装到 `{app}\Update\client_update.json`。
- `home` → `gen_update_json.ps1` 附在 `update.json` 尾部（`versions.json` 无 `home` 时整块省略，老 `update.json` 仍合法）。

派生链：

- `gen_version.bat` → `version.iss` + 调 `gen_json.ps1` 产出 `client_versions.json`（剔掉 `release` / `packageKillProcesses` / `updateJsonBases` / `releaseMirrors`，只留纯逐包版本表）和 `client_update.json`，两者均 UTF-8 无 BOM，由 iss 装进 `{app}\Update\`。
- `build_app.bat` 逐包 stage → `:packsha` 打 `<包名>-<包版本>.7z` 到 `Publish\releases\v<包版本>\` 并 `certutil` 算 sha256 记入 `pkglist.txt`（未变包命中历史目录直接 `[SKIP]` 不重打）→ `gen_update_json.ps1` 用 `pkglist.txt` + `versions.json` 生成 `Publish\releases\update.json`（`url` 写成 `v<该包版本号>/<文件名>`，包按 `APPLY_ORDER` 排列，字段定序）→ `build_inno_setup.bat` 出安装包。
- `release_app.bat` 只遍历上传 `Publish\releases\v<release>\` 里实际存在的 `.7z`（即本次变更包）+ installer，未变包沿用历史 release 的 asset；随后把 `releases/update.json` commit push 到 Publish 仓库，并 `purge.jsdelivr.net` 清 CDN。release 已存在则 `[SKIP]`，绝不覆盖资产（覆盖会让镜像缓存失配）。

于是 updater 端最终拼出的下载地址是：

```
真实：  https://github.com/jearry/yyzTools/releases/download/v<pkgVer>/<pkg>-<pkgVer>.7z
镜像：  https://ghfast.top/https://github.com/jearry/yyzTools/releases/download/v<pkgVer>/<pkg>-<pkgVer>.7z
```

`update.json` 自身则是 `<updateJsonBase>/releases/update.json`，即 Publish 仓库 `main` 分支的 `releases/update.json`，9 个源一起喂给 aria2 并行拉。

## 约定与坑

- ⚠️ **下载与解压刻意只依赖 `Platform` 下的第三方工具，不保留 WinHTTP / tar 回退**。GitHub 在国内慢是核心痛点，aria2 的多源并行 + 断点续传收益远大于「少一个外部依赖」。工具缺失就直接失败并记日志（`aria2c.exe not found` / `7z tool not found`）—— 代价是 `tools` 包（含 Aria2 与 7z）自身损坏会让更新彻底停摆。
- ⚠️ **7z 从 `Update\tools` 的副本运行**（复制的是工具 `7z.exe`/`7z.dll`，不是压缩包）。这是正确性处理不是 fallback：`tools` 包会覆盖 `Platform\7z\7z.exe`（`versions.json` 的 `packageKillProcesses.tools` 也把 `7z.exe` 列进 `killProcesses`）。不是「边解压边被换」——`ExtractArchive` 是 `WaitForSingleObject` 同步等完的；要规避的是 7z 进程刚退出、映像句柄尚未释放时 `ApplyStaging` 去 `MoveFileW` 覆盖它，那一失败就 `failures++`，`tools` 包版本永远升不上去。跑副本把这个窄窗口彻底消掉。
- ⚠️ **`--apply` 不等主进程优雅退出**，实际是无条件 `TerminateProcess` 掉 `yyzTools.exe`。时序由主进程侧保证 —— `PreRun` 是先拉起 updater 再 `return -3` 自行退出的。
- ⚠️ **参数是子串匹配**：`--check-and-apply` 字符串里含 `--apply`，靠先判前者规避；写错的参数（如 `-check`、`--appply`）不会报错，会静默落到默认 `--check` 分支。
- ⚠️ **先删 `update.ready` 再 `LaunchMain`**。反过来会有竞态：新主进程 `PreRun` 可能在 ready 被删掉之前检测到它，于是拉起第二个 `--apply` 并 `return -3` 自杀——而第二个 updater 要么撞上单例互斥体、要么已读不到 ready，都不会再 `LaunchMain`。**后果是主程序更新完起不来**（不是无限循环：删 ready 那句无论排前排后都会执行，且成功应用的包已被删，第二轮解压过不去）。
- ⚠️ **`DoCheck` 遇到下载/校验失败是 `break` 不是 `continue`**：剩余包本轮不再检查，已下好的包留在 `downloads` 等下次复用，`update.ready` 本轮不写，且**本轮不做孤儿清理**（见「孤儿下载包清理」）。
- ⚠️ **版本号只在整包应用零失败后才写回 `client_versions.json`**，且只有此时才删 `.7z`。部分失败 → 版本不升 + 包保留 → 下次检查复用缓存直接重试 apply。**但这条只保得住「远端版本没变」的情况** —— 远端一发新版，旧包就脱离计划成了孤儿，由 `CleanStaleDownloads` 兜底删除。
- ⚠️ **`yyzUpdater.exe.old` 删不掉是预期行为**，由下次启动清理（日志 `cleaned up yyzUpdater.exe.old`）。别把它当残留去手工处理逻辑。
- ⚠️ **单例互斥体是主进程判断「检测中」的唯一依据**。updater 崩溃时互斥体随进程释放，托盘状态自动回到 `idle`，无需额外心跳。
- ⚠️ **`update.json` 的 `url` 若写成数组，镜像前缀就不再参与拼接**（`BuildMultiSourceUrls` 不介入），数组元素被原样当完整多源列表。
- ⚠️ `gen_update_json.ps1` 的 `$order` 必须与 `updater.cpp` 的 `APPLY_ORDER` 保持**同一批包名**（脚本注释已标明）。脚本是 `foreach ($name in $order)` + `ContainsKey` 过滤，**不在 `$order` 里的包根本不会写进 `update.json`** —— 新增子包漏加这一行的后果是该包永远不下发，而不是顺序错乱（updater 侧把 `packages` 读进 `std::map` 再按 `APPLY_ORDER` 排序，json 里的文本顺序对它无意义，只影响人读 diff）。
- ⚠️ **发布侧两条硬约束**（违反则 url 404）：① 本次变更包的版本号必须等于 `release`，否则它被打进 `v<自己版本>/` 而 `release_app.bat` 只遍历 `v<release>/` → 漏传；② 未变包的版本号必须历史上作为某次 release tag 完整发过，否则拼出的 `v<pkgVer>/…` 指向不存在的 release。
- ⚠️ JSON 读写统一走 `nlohmann::ordered_json`（`library\3rd\include\nlohmann`，`IncludePath` 已含），序列化自动转义，路径反斜杠无需手工处理；解析用 `parse(text, nullptr, false)` 非异常模式，`is_discarded()` 判失败。与 yyzlib `Ptree.h` 同一风格。

## 构建产物

`updater.vcxproj`：SubSystem `Windows`，`stdcpp20`，Unicode，附加依赖 `winhttp.lib;bcrypt.lib;shlwapi.lib`。x64 输出 `bin64\yyzUpdater.exe`（Debug 配置 `TargetName` 为 `yyzUpdater_d`），**没有 PostBuildEvent** —— 它不进 `Platform\yyzTools\`（`build_app.bat` 打 `yyztools` 包时还显式 `del` 掉那里可能残留的 `yyzUpdater.exe`，避免与 `main` 包重复）。

`framework.h` 里的 `#include <winhttp.h>` 与链接的 `winhttp.lib` 是早期 WinHTTP 实现的残留，现无代码使用。

安装位置：iss 把 `..\bin64\yyzUpdater.exe` 装到 `{app}`（`%APPDATA%\yyzTools`）根，与 `yyzTools.exe` 同级；卸载/覆盖安装前 iss 会 `TaskKill('yyzUpdater.exe')`。升级则由 `main` 子包覆盖自身。

单元测试：`src/unit_tests/UpdaterVersionTest.cpp` 直接 include `../updater/version.h`，只覆盖 `CompareVersion`。
