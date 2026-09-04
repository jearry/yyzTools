# yyzInputHint

A keyboard-key hint and mouse-click highlighter tool: when a shortcut is pressed, it shows the key combination at the center of the screen; when the mouse is pressed, it draws a fading spotlight at the cursor. Input is collected purely with Win32 + RawInput; the mouse spotlight is drawn with Windows Composition (`winrt::Compositor` + `DesktopWindowTarget`). Invoked by the command module `Modules/App/yyzInputHint.zenmod` via `type:"app"` + `cmdPath:"yyzTools\\yyzInputHint.exe"`.

A Windows-subsystem program, a **long-lived process** (no tray, no UI): once started it listens for input until it is launched again (toggle semantics) or terminated by the main process. The command line accepts only one argument: `--area,x,y,w,h` (the keyboard-hint anchor rectangle, in physical screen pixels; passed in by yyzScreenRec during screen recording so the hint bar shows centered at the bottom of that rectangle).

## Launch and Exit

| Scenario | Path |
|----------|------|
| Launch | Command palette / dock / global hotkey → `CmdManager::Launch("inputHint")` → `Win32Manager::LaunchApp` → `LaunchExeShell` (the `cmdPath` of the `.zenmod` is already resolved into an absolute path under `Platform\yyzTools\` by `ModuleManager`) |
| Default hotkey | `Ctrl+1` (in the packaged default config `inno_setup/yyztools.json`, under `shortcutKeys.apps`, with `appId:"inputHint"` / `modifier:"2"` (MOD_CONTROL) / `virtualCode:"49"`) |
| Launch again | `CheckExistProcess()` (`Application.cpp`): the named mutex `yyzTools::yyzInputHint` already exists → launching with no argument = toggle semantics: `FindWindow(L"yyzTools::yyzInputHint")` gets the existing instance's window and `PostMessage(WM_QUIT)`, while the new instance `return -1` exits immediately; launching with `--area` exits directly **without closing the existing instance** (yyzScreenRec normally sends a message to switch in place first; this is only a fallback) |
| Screen-recording mode switch | yyzScreenRec (`src/screenrec/InputHintCtl.cpp`) sends `SendMessageTimeout(WM_COPYDATA)` to the highlight window, with `dwData` = `INPUTHINT_MSG_RECORD_MODE` (`WM_USER + 105`, `PubDefInput.h`), payload `RecordModePayload{enable, area}`: `enable=1` anchors the selection, `0` clears the anchor; returns TRUE to mean "handled" (older versions went through `DefWindowProc` returning FALSE, which the sender used to downgrade). The protocol constants exist in two copies across the two projects and must be changed in sync |
| Main process exit | `Application::Uninit()` calls `InputHint::Instance().Uninit()` (`src/public/InputHint.cpp`): `ExistExeProcess(L"yyzInputHint.exe")` + `TerminateProcessId`, killing them one by one |
| Install/Update | `inno_setup/yyztools.iss` runs `TaskKill('yyzInputHint.exe')`; during incremental updates `versions.json`'s `packageKillProcesses.yyztools.killProcesses` also includes it |

⚠️ Global hotkeys do not respond in game mode: `WinCmdPlate::HandleWindowMessage`'s `WM_HOTKEY` branch first checks `GET_CONFIG_INT(CONF_GAME_MODE)`.

## Input Collection (RawInput)

`RawInputListener` (shared implementation `src/public/RawInputListener.cpp`, the same copy as the main process / wallpaper; compiled in this project via the `NotUsing` pch method) creates a 0×0 `WS_POPUP | WS_EX_TOOLWINDOW` hidden window (class name `yyzTools::RawInputWindowClass`) as the target that receives `WM_INPUT`, and registers both the keyboard and mouse HID devices (`HID_USAGE_GENERIC_KEYBOARD` / `HID_USAGE_GENERIC_MOUSE`) with `RIDEV_INPUTSINK`, so it receives global input without needing focus.

`RAWMOUSE` is translated into equivalent window messages and handed to `MouseHighlighter::MouseProc`:

| RawInput source | Dispatched `WPARAM` |
|-----------------|---------------------|
| `usFlags == MOUSE_MOVE_RELATIVE` and `lLastX/lLastY` non-zero | `WM_MOUSEMOVE` |
| `RI_MOUSE_LEFT_BUTTON_DOWN` / `_UP` | `WM_LBUTTONDOWN` / `WM_LBUTTONUP` |
| `RI_MOUSE_RIGHT_BUTTON_DOWN` / `_UP` | `WM_RBUTTONDOWN` / `WM_RBUTTONUP` |
| `RI_MOUSE_MIDDLE_BUTTON_DOWN` / `_UP` | `WM_MBUTTONDOWN` / `WM_MBUTTONUP` |
| `RI_MOUSE_WHEEL` | `WM_MOUSEWHEEL` |

The keyboard uses `RI_KEY_BREAK` to judge press/release, and calls back `(vkCode, pressed)` to `KeyboardHintWindow::ProcessVkKey`.

`#define USE_RAW_INPUT 1` at the top of `Application.cpp` decides the collection method; set to 0 to switch to `KeyboardInput` (`WH_KEYBOARD_LL`) + `MouseInput` (`WH_MOUSE_LL`), two low-level hooks. Both paths are kept in the project, with matching callback signatures, so you can switch directly.

## Keyboard Hint Window (`KeyboardHintWindow`)

Window class name `KeyboardHintWindow`, `WS_POPUP` + `WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT`, `SetLayeredWindowAttributes` fixed at 60% opacity, `DwmSetWindowAttribute(DWMWA_WINDOW_CORNER_PREFERENCE, DWMWCP_ROUND)` enables Win11 rounded corners. Content is self-drawn with GDI: `Arial` 36px bold; the window size is computed from text width/height + padding (`+40` / `+20`), and positioned **bottom-centered** (`KeyboardHintWindow::Relayout`): when there is a recording anchor rectangle (`SetAnchorRect`, from the `--area` argument or the `INPUTHINT_MSG_RECORD_MODE` message) it sticks to the bottom-center of the selection; otherwise to the bottom-center of the primary screen (`SM_CXSCREEN` / `SM_CYSCREEN`).

Keys are split into three groups (the `switch` in `ProcessVkKey`), each with a `KeyStateMap` (`std::map<DWORD, std::wstring>`); pressing writes, releasing erases, and the label is taken in the system's localized name via `GetKeyName()`:

| Group | Members | Purpose |
|-------|---------|---------|
| System keys `s_pressedSysKeys` | `VK_CONTROL` / `VK_SHIFT` / `VK_MENU` (RawInput reports no left/right distinction), `VK_LCONTROL` / `VK_RCONTROL` / `VK_LSHIFT` / `VK_RSHIFT` / `VK_LMENU` / `VK_RMENU` (hook form), `VK_LWIN` / `VK_RWIN` | Modifier keys |
| Function keys `s_pressedFunKeys` | `VK_ESCAPE` `VK_TAB` `VK_CAPITAL` `VK_BACK` `VK_RETURN` `VK_HOME` `VK_END` `VK_PRIOR` `VK_NEXT` `VK_INSERT` `VK_DELETE` the four arrow keys `VK_NUMLOCK` `VK_PAUSE` `VK_SCROLL` `VK_PRINT` `VK_APPS` `VK_F1`–`VK_F12` | Shown even when pressed alone |
| Normal keys `s_pressedNormalKeys` | Everything else (`default` branch) | Shown only when combined with a system / function key |

The display decision is in `UpdateKeyboardState()`; the text is joined with ` + ` in the order "system keys + function keys + normal keys":

- System keys present + non-system keys present → show, and set `sys_ignore = true`.
- Only system keys and `sys_ignore == false` → show (pressing Ctrl alone also shows).
- No system keys but function keys present → show.
- Only normal keys → **do not show** (normal typing will not pop up a window).
- When not required to show: if `sys_ignore` is true, keep calling `Show(last_displayText)` to preserve the original combination text, and simultaneously `StartTimer()`; when the `KEYBOARD_HINT_TIMER_ID` timer reaches `KEYBOARD_HINT_DELAY_MS` (1000ms) it calls `Hide()`.

## Mouse Click Highlight (`MouseHighlighter`)

Window class name `yyzTools::yyzInputHint` (same as the single-instance mutex), `WS_POPUP` + `WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOOLWINDOW`, `WM_NCHITTEST` always returns `HTTRANSPARENT` to guarantee click-through. Composition tree: `ContainerVisual` root (`RelativeSizeAdjustment{1,1}`) → `ShapeVisual` → on each press `Append` a `CompositionSpriteShape` (`CreateEllipseGeometry`), and on release use `CreateColorKeyFrameAnimation` to animate the color alpha to 0 for the fade-out.

`Application::PreRun()` calls `SwitchActivationMode()` once immediately after startup (`PostMessage(WM_SWITCH_ACTIVATION_MODE)`, value `WM_APP`) → `StartDrawing()`, so the highlighter is on as soon as the process starts.

Fixed parameters (`PubDefInput.h`, currently no configuration entry, all compile-time constants):

| Constant | Value | Description |
|----------|-------|-------------|
| `MOUSE_HIGHLIGHTER_DEFAULT_RADIUS` | `20` | Spotlight radius |
| `MOUSE_HIGHLIGHTER_DEFAULT_DELAY_MS` | `500` | Fade animation delay |
| `MOUSE_HIGHLIGHTER_DEFAULT_DURATION_MS` | `250` | Fade animation duration |
| `MOUSE_HIGHLIGHTER_DEFAULT_LEFT_BUTTON_COLOR` | ARGB `166,255,255,0` | Left button yellow |
| `MOUSE_HIGHLIGHTER_DEFAULT_RIGHT_BUTTON_COLOR` | ARGB `166,0,0,255` | Right button blue |
| `MOUSE_HIGHLIGHTER_DEFAULT_ALWAYS_COLOR` | ARGB `0,255,0,0` | Persistent pointer color, **alpha is 0** |

The three switches `m_leftPointerEnabled` / `m_rightPointerEnabled` / `m_alwaysPointerEnabled` are hardcoded to `true` in the header.

## Theme Following

Only the keyboard hint window needs coloring (the mouse spotlight uses functional colors and does not follow the theme); the palette is cached in the `Theme` namespace:

| Theme | `Palette{bg, text}` |
|-------|---------------------|
| Dark `kDark` | `RGB(1,1,1)` / `RGB(255,255,255)` |
| Light `kLight` | `RGB(249,249,249)` / `RGB(26,26,26)` |

Decision order (`Theme::IsDarkMode()`): the `Theme` field of shared memory `SHARED_MEM_NAME_YYZTOOLS` (`light` / `dark`, written by the main process) → if `system` or empty, read the registry `HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\AppsUseLightTheme` (`0` dark; treat as light if missing/failed).

Refresh timing: `KeyboardHintWindow::Init` does the first `Theme::Refresh()`; `WM_SETTINGCHANGE` with `lParam == L"ImmersiveColorSet"` (system dark/light switch); `INPUTHINT_MSG_THEME_CHANGE` (`WM_USER + 104`, same value as `WM_THEME_CHANGE_MSG` in `src/public/PubDefWin.h`). The latter is sent by the main process's `ConfigProcess.cpp` `NotifyInputHintThemeChanged()`: `EnumWindows` matches the **hardcoded class name** `KeyboardHintWindow` then `PostMessage`; the message carries no value, so on receipt it re-reads the shared memory.

## Conventions and Pitfalls

- Quitting the old instance relies on `PostMessage(hwnd, WM_QUIT, 0, 0)` (`Application.cpp:33`); inside `RunMessagePump()` you do **not** need to re-check `msg.message == WM_QUIT` in the loop body: `GetMessage` returns 0 when it receives `WM_QUIT` (judged by message value; `PostMessage` / `PostThreadMessage` / `PostQuitMessage` all behave the same), and `while (GetMessage(...))` already exits on its own.
- ⚠️ **The single-instance mutex name and the mouse-highlight window class name are the same string** `yyzTools::yyzInputHint`, and the quit signal is sent to the highlight window; whereas the theme notification is sent to a **different** window class, `KeyboardHintWindow`. If you rename either, you must change the caller at the other site (the latter is also written in the main process `ConfigProcess.cpp`).
- **RawInput devices are unregistered in pairs**: `RawInputListener::Uninit()` changes the `dwFlags` of `m_devices` to `RIDEV_REMOVE` and nulls `hwndTarget`, then re-calls `RegisterRawInputDevices`, then `DestroyWindow` + `UnregisterClass`. Unregistration is done before destroying the window — keep this order as-is.
- ⚠️ **A transparent window must not cover the entire virtual screen**: the three `SetWindowPos` calls in `AddDrawingPoint` / `StartDrawing` / `BringToFront` deliberately use `+1` for the origin and `-2` for the size (`SM_XVIRTUALSCREEN + 1`, `SM_CXVIRTUALSCREEN - 2`); the comment notes this is a workaround for the system glitch where a full-screen transparent window garbles the taskbar's transparency effect. Change all three together.
- ⚠️ **After clicking a topmost window you must grab back the Z-order via a timer**: on left/right button press it starts the `BRING_TO_FRONT_TIMER_ID` 10ms timer; the comment explains that a topmost window will cover the highlight layer within 0–30ms, so it fires 5 times in a row (~50ms) before `KillTimer`; using a flat 50ms period would flicker.
- ⚠️ **A fade duration of 0 makes the animation no-op**, so `StartDrawingPointFading` forcibly changes `m_fadeDuration_ms` / `m_fadeDelay_ms` of 0 to 1ms.
- In `GetKeyName()`, the `Left` / `Right` prefix trimming thresholds `str.size() > 4` / `> 5` are deliberate: the arrow-key names themselves are `Left` (4) / `Right` (5), which exactly avoids trimming and are kept, while only things like `Left Ctrl` / `Right Shift` get trimmed to `Ctrl` / `Shift`. Changing the threshold would trim the arrow-key names to empty strings.
- For `VK_INSERT` `VK_DELETE` `VK_HOME` `VK_END` `VK_NEXT` `VK_PRIOR` `VK_NUMLOCK` `VK_APPS` `VK_LWIN` `VK_RWIN`, `GetKeyName()` manually adds the extended bit `0x100` before passing to `GetKeyNameText`, otherwise it would get the same-scancode name on the numpad.
- ⚠️ RawInput and low-level hooks report different modifier-key VKs: RawInput gives `VK_CONTROL` / `VK_SHIFT` / `VK_MENU`, while `WH_KEYBOARD_LL` gives the left/right-separated forms `VK_LCONTROL` / `VK_RCONTROL`, etc. `ProcessVkKey`'s system-key branch lists both sets in the same `case` group; do not delete them when switching `USE_RAW_INPUT`.
- ⚠️ Mouse movement only handles `usFlags == MOUSE_MOVE_RELATIVE`; VMs / RDP report `MOUSE_MOVE_ABSOLUTE`, so in such environments the spotlight does not follow while dragging (mousefinder is unaffected — it does not parse RawInput coordinates and always takes the position fresh via `GetCursorPos`).
- ⚠️ `MouseHighlighter::Uninit()` does `TryEnqueue(DestroyWindow)` into the `DispatcherQueue`, but it is called by `Application::PostRun()` **after** the message loop exits, at which point there is no loop to dispatch the task, so the window is actually reclaimed when the process exits.
- `MouseHighlighter::AddDrawingPoint` has a `TODO: We're leaking shapes for long drawing sessions.` — shapes are only `Shapes().Clear()`'d at `StartDrawing`/`StopDrawing`, so rapid clicking over a long session accumulates.
- Cursor position always uses `GetCursorPos` + `ScreenToClient` (RawInput only gives deltas, no absolute position); `MouseHighlighter::Init` first calls `SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`, and `app.manifest` also declares `PerMonitorV2` — both places are present, so do not change only one.
- Both window classes handle "reuse if already registered": `MouseHighlighter::Init` first probes with `GetClassInfoW`, and `KeyboardHintWindow::RegisterWindowClass` tolerates `ERROR_CLASS_ALREADY_EXISTS`.
- Linking of libraries is via `#pragma comment` in the source: in `pch.cpp` it is `Dwmapi.lib` / `coremessaging.lib` / `runtimeobject.lib` (`coremessaging` is needed for `CreateDispatcherQueueController`); the vcxproj lists no additional dependencies.

## Build Output

`OutDir` is `bin64\` (x64; the Win32 config outputs to `bin\`), language standard `stdcpp20`, subsystem `Windows`. After building, a PostBuildEvent copies a copy:

```
copy /Y $(SolutionDir)..\bin64\yyzInputHint.exe $(SolutionDir)..\bin64\platform\yyzTools\yyzInputHint.exe
```

At runtime it uses `bin64/Platform/yyzTools/yyzInputHint.exe` (`GetAppDir() + "\\Platform\\yyzTools"` in `src/public/InputHint.cpp`, which is also the base directory of `cmdPath` in the `.zenmod`). When packaging, it is recursively pulled into the installer by `inno_setup/yyztools.iss`'s `..\bin64\Platform\*`; incremental updates belong to the `yyztools` sub-package of `versions.json` (`bin64\Platform\yyzTools\`).
