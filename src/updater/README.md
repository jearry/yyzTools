# yyzUpdater

The auto-updater for yyzTools: fetch the remote `update.json` and compare versions package by package → download from multiple sources via aria2 → verify with sha256 → extract with 7z → replace files → restart the main program. Launched by the main process `yyzTools.exe` via the command line (`Application.cpp`'s periodic silent check, the pending-apply check in `Application::PreRun`, and the tray-triggered manual calls in `NativeApi::CheckUpdate` / `NativeApi::ApplyUpdate`).

A Windows-subsystem program with **no UI throughout** (no window, no MessageBox, no progress bar); progress is only written to the log; single-instance (named mutex `Local\yyzUpdater_SingleInstance`), exits when done. Exit codes: `0` success, `1` failure.

## Usage

```
yyzUpdater.exe [--check | --apply | --check-and-apply]
```

`Main.cpp` concatenates `argv[1..]` into a single string and judges via `std::wstring::find` **substring matching**; the judgment order is the priority:

| Argument | Entry | Behavior |
|----------|-------|----------|
| `--check-and-apply` | `RunCheckAndApply` | First clears `update.result`; if `update.ready` already exists, apply directly; otherwise check, and apply on any update, or write the `update.result` marker on no update / failure |
| `--apply` | `RunApply` | Read `update.ready` → kill processes → extract → replace → restart `yyzTools.exe` |
| Other / no argument | `RunCheck` | Default branch: check + download + verify + write `update.ready`, then exit without touching any installed files |

The `--key=value` form is not supported, and there are no other arguments.

## Directory Layout

The install directory is the update target: `%APPDATA%\yyzTools` (`inno_setup/yyztools.iss`'s `DefaultDirName={userappdata}\{#SoftIdent}`, `PrivilegesRequired=lowest`, so the whole update flow does not need elevation by default).

| Path | Resolver | Content |
|------|----------|---------|
| `%APPDATA%\yyzTools` | `GetInstallDir()` | Replace-target root; also the base to locate `Platform\Aria2\aria2c.exe` and `Platform\7z\7z.exe` |
| `…\Update` | `GetUpdateDir()` | `client_versions.json`, `client_update.json`, `update.json`, `update.ready`, `update.result` |
| `…\Update\downloads` | `GetDownloadDir()` | `<package>-<version>.7z` persistent cache (kept across processes, reused with sha256) |
| `…\Update\staging\<package>` | `GetStagingDir()` | Extraction staging; one subdir per package, deleted after applying |
| `…\Update\tools` | `GetToolsDir()` | Runtime copy of `7z.exe` / `7z.dll` |
| `%APPDATA%\yyzTools\logs\updater.log` | `..\public\logger.cpp` | Log |

`EnsureDir` (`updater.cpp`) is a wrapper that returns the path from `EnsureDirTree` (`applying.cpp`, created level by level), so `Update` and its subdirectories can be created in one shot even if the parent is missing. `GetUpdateDir` goes through `GetAppDataRoot() + "\\Update"` and does **not** go through `GetInstallDir`; the two do not depend on each other.

## Update Check (`DoCheck`)

- **update.json source**: `updateJsonBases` in `client_update.json`, each base joined with `/releases/update.json` to form the multi-source list. When the config file does not exist, fall back to `FALLBACK_UPDATE_JSON_BASE` (`https://raw.githubusercontent.com/jearry/yyzTools/main`) and write this default back to the file for the user to edit.
- **Local version**: only reads `Update\client_versions.json` (a plain `{package: version}` dict), **not** the version resource of the exe. This file is laid down by the installer and later overwritten by the updater itself via `SaveVersions`.
- **Compare**: `CompareVersion` (`version.h`) does a numeric comparison segment by segment on `.`, zero-padding the shorter side; `remote <= local` skips that package.
- **update.json structure**: top-level `packages` is an object keyed by package name; each package has `version` / `url` / `sha256` / `killProcesses` / `killProcessesAdmin` / `stopService`; the top level may also have a `home` block unrelated to updating (see below).
- **url composition**: when `url` is a string it holds only the suffix `v<that package's version>/<filename>`, and the updater prepends `RELEASE_BASE` (`https://github.com/jearry/yyzTools/releases/download`); if the suffix itself starts with `http://` / `https://` it is used as-is as a complete address. `url` also accepts an array form, whose elements are treated as-is as a multi-source list.
- **Mirrors**: `BuildMultiSourceUrls` prepends each `releaseMirrors` entry as a prefix `<mirror>/<realUrl>`; the real address is always first.
- **Ordering**: the matched packages are sorted by `APPLY_ORDER` and then written to `update.ready`: `web` → `modules` → `wallpaper` → `yyztools` → `tools` → `yyzfilesearch` → `rapidocr` → `ffmpeg` → `main` (resource classes first; `main` includes self-update so it is last).
- **Result**: all up to date → `kUpToDate`; `update.ready` written → `kReady`; any package download/verify failure → `kError`.
- The `update.json` text is cached at `Update\update.json`; the main process's `NativeApi::GetHomeAd` (`Zen.getHomeAd`) reads its `home` block as the command-palette home-page ad-slot config — so this file is not a pure temp file.

## Download (`http_client.cpp`)

All downloads go through the `Platform\Aria2\aria2c.exe` subprocess (`RunAria2`, `CREATE_NO_WINDOW` + `SW_HIDE`), with fixed arguments:

```
--no-conf --console-log-level=warn --summary-interval=0
--allow-overwrite=true --auto-file-renaming=false
--split=16 --max-connection-per-server=16 --min-split-size=1M
--max-tries=3 --retry-wait=2 --timeout=60 --connect-timeout=30
--dir=<directory> --out=<filename> <url1> <url2> ...
```

- Multiple urls are handed to aria2 **at once**, meaning "pull and merge the same file from multiple sources in parallel", not a fallback retry one by one.
- Timeouts: `DownloadText` (update.json) 30s, `DownloadFile` (package) 300s; on timeout it `TerminateProcess` and returns non-zero, abandoning this round for next time.
- **Do not pre-delete existing files**: relies on aria2's `.aria2` control file for resume; failed partial files are deliberately kept (log `partial kept for resume`).
- `DownloadFile`'s `progress` parameter is kept but not wired (headless, does not parse aria2 output).
- `DownloadText` lets aria2 `--out=update.json` land in the `Update` root, and the return value is then read from the file by `DoCheck`.

## Verification (`hasher.cpp`)

- `Sha256File`: CNG `BCryptOpenAlgorithmProvider(BCRYPT_SHA256_ALGORITHM)` streaming hash, 64KB buffer, lower-case hex output; links `bcrypt.lib`.
- Compared to `update.json`'s `sha256` via `_stricmp` (case-insensitive).
- Before downloading, if the same package already exists in `downloads`, compute its sha256 once; on a hit, `reuse cached package, skip download`.
- On verification failure: **the file is not deleted**; mark failure and abort this round, to be re-downloaded by aria2 next time.

## Orphan Download-Package Cleanup (`CleanStaleDownloads`)

Package filenames carry the version (`<package>-<version>.7z`), but `DeleteFileW` on a successful apply only removes the exact path `p.zipPath`. So **once the remote version jumps, the package that failed to apply last round becomes a permanent orphan** — after a failed 1.0.4 upgrade the server ships 1.0.5 directly, and the `*-1.0.4.*.7z` in `downloads` is no longer referenced by any plan; even a successful 1.0.5 apply only deletes its own few. The `.7z.aria2` resume-control files are the same (aria2 deletes them on download completion; interrupted ones stay forever).

Therefore `DoCheck` finalizes with a whitelist after the version is fixed: it scans `downloads`, keeps this round's pending packages' `<filename>` and `<filename>.aria2`, and deletes everything else.

- **When there is no update (`kUpToDate`) the whitelist is empty**, and the whole directory is cleared — in that state any leftover is an orphan.
- ⚠️ **The failure path (`failed`) deliberately does not clean up**: when `DoCheck` hits a download/verify failure it `break`s, so `pending` only covers up to the failed package and the rest are never even checked. Using this incomplete list as the whitelist would wrongly delete caches that should be reused. Leave cleanup to the next successful check round.
- Deletion failures (e.g. file in use) are only logged, not aborted, and retried next round.

## Extraction (`zip_extract.cpp`)

`ExtractArchive` calls `Platform\7z\7z.exe`:

```
7z.exe x "<archive>" -o"<destDir>" -y -bd
```

`CREATE_NO_WINDOW` + `SW_HIDE`, `TerminateProcess` after 180s timeout; a non-zero exit code counts as failure. Before running, `Platform\7z\7z.exe` and `7z.dll` are `CopyFileW`'d to `Update\tools` and launched from the copy (reason in "Conventions and Pitfalls"). `RunApply` calls `RemoveDirRecursive(staging)` before extraction to ensure a clean directory.

## Apply Update (`applying.cpp` / `RunApply`)

Sequence:

1. `KillChildProcesses()` — currently only kills `yyzTools.exe` (other child processes are declared per-package via `killProcesses` and killed per package).
2. For each package (already sorted by `APPLY_ORDER`): `KillPackageProcesses(killProcesses, killProcessesAdmin)` → stop services declared by `stopService` → clear staging → `ExtractArchive` → `ApplyStaging` → delete staging.
3. Only when that package's `ApplyStaging` returns true (zero failures) does `UpdateLocalVersion` write back `client_versions.json` and delete the `.7z`; otherwise the version does not advance and the package is kept for retry next time.
4. After the loop ends, **first** `DeleteFileW(update.ready)`, **then** `LaunchMain(appDir)` starts `%APPDATA%\yyzTools\yyzTools.exe` (working directory is appDir, no arguments).

`StopServiceByName` / `StartServiceByName`: `stopService` lists the service names to stop before applying that package (e.g. `yyzFileSearchSvc`). With normal privileges SCM stops them directly; only when privileges are insufficient (the service runs as LocalSystem) does it go through UAC runas to call `sc.exe stop/start`. Actually-stopped services are recorded in a set, and after all packages are applied they are uniformly restarted via `StartServiceByName`.

`KillPackageProcesses`: `killProcesses` terminates by image name (`FindProcessIds` returns **all** PIDs with the same name, killing multiple instances together); `killProcessesAdmin` first tries a normal kill, and only if still found does `ShellExecuteExW("runas", "taskkill.exe", "/f /im <exe>")` and waits 5 seconds — the liveness probe first avoids a pointless UAC popup when the process is not actually there. It `Sleep(500)` only when a kill actually happened.

`ApplyStaging`: `CollectFiles` recursively collects all files in staging (with relative paths), and for each file:

- Target directory does not exist → `EnsureDirTree` creates level by level (`CreateDirectoryW` can only create one level, so newly-added multi-level directories inside a package rely on it).
- Target exists → first `MoveFileW` to `<dest>.bak` (its own exe uses `.old`), deleting any same-named residue before moving.
- `MoveFileW` the new file to the target; on failure downgrade to `CopyFileW` + `DeleteFileW` (log `move failed, copy fallback ok`).
- Success and not itself → `.bak` goes into a to-delete list, deleted uniformly after all files are processed; failure → `MoveFileW` moves `.bak` back to its original place (**per-file rollback, not whole-package rollback**), `failures++`.
- Returns `failures == 0`.

**Self-update**: `main` contains `yyzTools.exe` + `yyzUpdater.exe` + `WebView2Loader.dll`, and `APPLY_ORDER` puts it last. `yyzUpdater.exe` is the image of the currently running process — Windows allows renaming a running exe but not deleting/overwriting it, so it is renamed to `yyzUpdater.exe.old` to make room, and the new version is moved in; `.old` is not put into the to-delete list (it cannot be deleted at this moment) and is cleaned up by `DeleteFileW` at the top of `Main.cpp` on next startup.

## Logging (`..\public\logger.cpp`, shared module)

`wWinMain`'s first action is `LogInit(%APPDATA%\yyzTools\logs, updater.log)` (earlier than the singleton check, so "an instance is already running" is still logged). `Log` / `LogFmt` (`vsnprintf`, single-entry cap 2048 bytes) append to `logs\updater.log` with a `[YYYY-MM-DD HH:MM:SS] ` prefix, each entry independently open/append/close. When it exceeds `kMaxLogSize` (10MB) it is rotated via `MoveFileExW` to `updater.log.1` (overwriting the old backup, only one kept).

## Integration with the Main Process

| Trigger | Code location | Command |
|---------|---------------|---------|
| Silent check 15s after startup, once every 24h | `src/yyztools/Application.cpp` (`CONF_AUTO_UPDATE_ENABLED` / `CONF_LAST_UPDATE_CHECK`, `DelayedTask`) | `--check` |
| `Update\update.ready` found on startup | `Application::PreRun`, after launching it `return -3` so the main process exits and yields | `--apply` |
| Tray "Check for updates" | `NativeApi::CheckUpdate` (`Zen.checkUpdate`) | `--check-and-apply` |
| Tray "Update now" (ready state) | `NativeApi::ApplyUpdate` (`Zen.applyUpdate`) | `--apply` |

State readback goes through `NativeApi::GetUpdateState` (`Zen.getUpdateState`, polled every 2s by `src/web/tray/tray.js`): `OpenMutexW` detects `Local\yyzUpdater_SingleInstance` → `checking`; `update.ready` exists → `ready`; otherwise `idle`, and it reads-then-deletes `update.result` once (content `uptodate` / `error`). `update.result` is only written on the `--check-and-apply` path — a silent `--check` does not write it, otherwise the user might see a spurious "up to date" popup next time they open the tray.

## Release-Side Integration

The single source of truth is `versions.json` at the repo root:

- `release` is the **release identity number**: the GitHub release tag `v<release>`, the installer filename, and `inno_setup/version.iss` are all derived from it; it must be incremented on every release.
- The 9 sub-packages `web` / `modules` / `wallpaper` / `yyztools` / `tools` / `yyzfilesearch` / `rapidocr` / `ffmpeg` / `main` each have their own version number representing that package's content version; if unchanged it does not advance. `main` is only the version of the main-program sub-package, not the release number.
- `packageKillProcesses.<package>.killProcesses|killProcessesAdmin|stopService` → into the same-named field of `update.json` → into `update.ready` → consumed by `KillPackageProcesses` / `StopServiceByName`.
- `updateJsonBases` (9 sources: four jsDelivr mirrors / raw.githubusercontent / raw.gitmirror / ghfast / ghproxy / gh-proxy) and `releaseMirrors` (`ghfast.top` / `ghproxy.com` / `gh-proxy.com`) → extracted by `gen_json.ps1` and written to `inno_setup/client_update.json` → installed by iss to `{app}\Update\client_update.json`.
- `home` → appended by `gen_update_json.ps1` to the tail of `update.json` (if `versions.json` has no `home`, the whole block is omitted; old `update.json` remains valid).

Derivation chain:

- `gen_version.bat` → `version.iss` + calls `gen_json.ps1` to produce `client_versions.json` (strips `release` / `packageKillProcesses` / `updateJsonBases` / `releaseMirrors`, keeping only the pure per-package version table) and `client_update.json`; both are UTF-8 without BOM, installed by iss into `{app}\Update\`.
- `build_app.bat` stages each package → `:packsha` packs `<package>-<package version>.7z` to `Publish\releases\v<package version>\` and uses `certutil` to compute sha256 into `pkglist.txt` (unchanged packages hit the historical directory and are directly `[SKIP]`'d without re-packing) → `gen_update_json.ps1` uses `pkglist.txt` + `versions.json` to generate `Publish\releases\update.json` (url written as `v<that package's version>/<filename>`, packages ordered by `APPLY_ORDER`, fields deterministically ordered) → `build_inno_setup.bat` produces the installer.
- `release_app.bat` only traverses and uploads the actually-existing `.7z` in `Publish\releases\v<release>\` (i.e. the packages changed this time) + the installer; unchanged packages reuse the assets of historical releases; then it commits and pushes `releases/update.json` to the Publish repo and purges `purge.jsdelivr.net` to clear the CDN. If the release already exists it is `[SKIP]`'d — never overwrite assets (overwriting would desync the mirror cache).

So the download address finally assembled on the updater side is:

```
Real:    https://github.com/jearry/yyzTools/releases/download/v<pkgVer>/<pkg>-<pkgVer>.7z
Mirror:  https://ghfast.top/https://github.com/jearry/yyzTools/releases/download/v<pkgVer>/<pkg>-<pkgVer>.7z
```

`update.json` itself is `<updateJsonBase>/releases/update.json`, i.e. `releases/update.json` on the Publish repo's `main` branch, fed to aria2 for parallel pulling from all 9 sources.

## Conventions and Pitfalls

- ⚠️ **Download and extraction deliberately only depend on the third-party tools under `Platform`, with no WinHTTP / tar fallback kept.** GitHub being slow in China is the core pain point; aria2's multi-source parallel + resume benefit far outweighs "one fewer external dependency". If a tool is missing it just fails and logs (`aria2c.exe not found` / `7z tool not found`) — the cost is that if the `tools` package (which contains Aria2 and 7z) itself is corrupted, updates stop entirely.
- ⚠️ **7z runs from the copy in `Update\tools`** (it copies the tool `7z.exe`/`7z.dll`, not the archive). This is a correctness measure, not a fallback: the `tools` package overwrites `Platform\7z\7z.exe` (and `versions.json`'s `packageKillProcesses.tools` also lists `7z.exe` in `killProcesses`). It is not "overwritten while extracting" — `ExtractArchive` synchronously waits via `WaitForSingleObject`; what it avoids is that right after the 7z process exits, while its image handle is not yet released, `ApplyStaging` does `MoveFileW` to overwrite it — that failure would `failures++` and the `tools` package version would never advance. Running from a copy eliminates this narrow window entirely.
- ⚠️ **`--apply` does not wait for the main process to exit gracefully**; it unconditionally `TerminateProcess`es `yyzTools.exe`. The timing is guaranteed on the main-process side — `PreRun` launches the updater first then `return -3` to exit on its own.
- ⚠️ **Arguments are substring-matched**: the string `--check-and-apply` contains `--apply`, avoided by checking the former first; a mistyped argument (e.g. `-check`, `--appply`) does not error and silently falls into the default `--check` branch.
- ⚠️ **Delete `update.ready` first, then `LaunchMain`**. The reverse has a race: the new main process's `PreRun` might detect `update.ready` before it is deleted, thus launching a second `--apply` and `return -3` to kill itself — while that second updater either hits the singleton mutex or can no longer read `update.ready`, so neither will `LaunchMain` again. **The consequence is the main program fails to start after updating** (not an infinite loop: the delete-ready line executes regardless of order, and the successfully-applied packages have been deleted, so the second round's extraction fails).
- ⚠️ **`DoCheck` `break`s, not `continue`s, on a download/verify failure**: the remaining packages are not checked this round, already-downloaded packages stay in `downloads` for reuse next time, `update.ready` is not written this round, and **no orphan cleanup happens this round** (see "Orphan Download-Package Cleanup").
- ⚠️ **The version number is only written back to `client_versions.json` after the whole package applies with zero failures**, and only then is the `.7z` deleted. Partial failure → version does not advance + package kept → next check reuses the cache to retry apply directly. **But this only holds when "the remote version has not changed"** — once the remote ships a new version, the old package falls out of the plan and becomes an orphan, cleaned up as a fallback by `CleanStaleDownloads`.
- ⚠️ **Failing to delete `yyzUpdater.exe.old` is expected behavior** and is cleaned up on next startup (log `cleaned up yyzUpdater.exe.old`). Do not treat it as a leftover and handle it manually.
- ⚠️ **The singleton mutex is the main process's only basis for judging "checking"**. When the updater crashes the mutex is released with the process, and the tray state automatically returns to `idle`, with no extra heartbeat needed.
- ⚠️ **If `update.json`'s `url` is written as an array, the mirror prefix no longer participates in the composition** (`BuildMultiSourceUrls` does not intervene), and the array elements are treated as-is as the complete multi-source list.
- ⚠️ `gen_update_json.ps1`'s `$order` must keep **the same set of package names** as `APPLY_ORDER` in `updater.cpp` (the script comment already states this). The script is `foreach ($name in $order)` + `ContainsKey` filtering, so **a package not in `$order` is never written into `update.json`** — the consequence of adding a new sub-package but forgetting this line is that the package is never delivered, not that the order is wrong (the updater side reads `packages` into a `std::map` then sorts by `APPLY_ORDER`; the text order in the json is meaningless to it, only affecting human-diff readability).
- ⚠️ **Two hard constraints on the release side** (violating them yields a url 404): ① the version number of this round's changed packages must equal `release`, otherwise it gets packed into `v<its own version>/` while `release_app.bat` only traverses `v<release>/` → missed upload; ② the version number of unchanged packages must have been fully released historically as some release tag, otherwise the assembled `v<pkgVer>/…` points to a non-existent release.
- ⚠️ JSON read/write uniformly uses `nlohmann::ordered_json` (`library\3rd\include\nlohmann`, already in `IncludePath`), serialization auto-escapes so backslashes in paths need no manual handling; parsing uses `parse(text, nullptr, false)` in non-throwing mode, and `is_discarded()` to judge failure. Same style as yyzlib's `Ptree.h`.

## Build Output

`updater.vcxproj`: SubSystem `Windows`, `stdcpp20`, Unicode, additional dependencies `winhttp.lib;bcrypt.lib;shlwapi.lib`. x64 outputs `bin64\yyzUpdater.exe` (Debug config `TargetName` is `yyzUpdater_d`), **with no PostBuildEvent** — it does not go into `Platform\yyzTools\` (`build_app.bat` explicitly `del`s any possibly-leftover `yyzUpdater.exe` there when packing the `yyztools` package, to avoid duplication with the `main` package).

The `#include <winhttp.h>` in `framework.h` and the linked `winhttp.lib` are leftovers from an early WinHTTP implementation; no code uses them now.

Install location: iss installs `..\bin64\yyzUpdater.exe` to `{app}` (`%APPDATA%\yyzTools`) root, alongside `yyzTools.exe`; before uninstall/overwrite install, iss runs `TaskKill('yyzUpdater.exe')`. Upgrades are handled by the `main` sub-package overwriting itself.

Unit tests: `src/unit_tests/UpdaterVersionTest.cpp` directly includes `../updater/version.h` and only covers `CompareVersion`.
