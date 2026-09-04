# yyzCmd

A command-line style Windows system command executor, implemented purely with the Win32 API (zero third-party dependencies). Invoked by the command module `.zenmod` via `type:"app"` + `cmdPath:"yyzTools\\yyzCmd.exe"`.

A Windows-subsystem, windowless GUI program (to avoid a black flash window when triggered by `LaunchExeShell`); it exits immediately after executing the command.

## Usage

```
yyzCmd.exe <command> [arguments]
yyzCmd.exe help          # Show usage in a popup
```

Exit codes: `0` success, `1` execution failed, `2` unknown command.

> ⚠️ **A missing argument does not yield a uniform exit code** (hardcoded per entry in the table in `CommandDispatch.cpp`): `theme` with no argument or an invalid argument returns `2`, while `volume` / `open` with no argument return `1`; `volume-up` / `volume-down` with no argument are not treated as errors and run with the default step of 5. Do not treat `2` as a uniform "missing argument" signal.

## Command List

| Area | Command | Description |
|------|---------|-------------|
| Power | `shutdown` | Shut down (`ExitWindowsEx` EWX_SHUTDOWN\|EWX_POWEROFF) |
| Power | `restart` | Restart |
| Power | `logoff` | Log off |
| Power | `lock` | Lock (`LockWorkStation`) |
| Power | `sleep` | Sleep (`SetSuspendState(FALSE,...)`) |
| Power | `hibernate` | Hibernate (`SetSuspendState(TRUE,...)`) |
| Monitor | `monitor-off` | Turn off monitor (`SC_MONITORPOWER`) |
| Monitor | `screensaver` | Launch screen saver (`SC_SCREENSAVE`) |
| Recycle Bin | `empty-recyclebin` | Empty the recycle bin (`SHEmptyRecycleBin`) |
| Volume | `volume <0-100>` | Set to a percentage (`IAudioEndpointVolume`) |
| Volume | `volume-up [step]` | Increase, default 5 |
| Volume | `volume-down [step]` | Decrease, default 5 |
| Volume | `mute` / `unmute` / `mute-toggle` | Mute control |
| Open Directory | `open <this-pc\|recyclebin\|downloads\|documents\|desktop\|pictures\|videos\|music>` | Open with Explorer (This PC / Recycle Bin use the shell GUID, user folders use `SHGetKnownFolderPath`) |
| Open hosts | `open hosts` | Open `%SystemRoot%\System32\drivers\etc\hosts` with Notepad (the path is resolved to an absolute path via `GetSystemDirectoryW` before being passed to `ShellExecuteW`, because `%VAR%` in `lpParameters` is not expanded) |
| System | `clear-clipboard` | Clear the clipboard |
| System | `theme <light\|dark\|toggle>` | Set/toggle the system theme (`light`/`dark` specified directly, `toggle` switches; writes the registry + broadcasts `ImmersiveColorSet`). `toggle-theme` is a compatibility alias |
| System | `eject-usb` | Safely eject removable disks (`SetupAPI` + `CM_Query_And_Remove_SubTree`), see below |

`eject-usb` is the **only command that requires UAC**: `CM_Query_And_Remove_SubTree` needs administrator privileges. When not elevated, it restarts itself via `ShellExecuteExW` with `runas` and immediately `return 0` (only the elevated instance does the actual work; the original process does not wait for the result). Enumeration uses `DIGCF_ALLCLASSES | DIGCF_PRESENT` to get all devices, then filters by a two-level condition: `CM_DRP_CLASS` must be `DiskDrive` and `CM_DRP_REMOVAL_POLICY` must be `EXPECT_ORDERLY_REMOVAL` / `EXPECT_SURPRISE_REMOVAL`, thereby avoiding input devices such as USB keyboards and mice. What gets removed is the **parent node** of the disk node (the USB mass-storage device) subtree; otherwise the entire USB drive will not disconnect. Returns `1` when nothing was ejected.

## Deployment Location

After building, a PostBuildEvent copies it to `bin64/Platform/yyzTools/yyzCmd.exe` (same directory as self-built tools such as yyzBrowser.exe), and it is packaged recursively by inno_setup's `Platform\*`.
