# yyzMouseFinder

Quickly locate the mouse cursor: dim the whole screen + draw a white spotlight at the cursor, with the spotlight shrinking from a 9× radius down to the target radius, then fading out once the mouse stops or any key is pressed. Drawn with Windows Composition (`winrt::Compositor` + `DesktopWindowTarget`); the mouse is tracked via RawInput. Invoked by the command module `Modules/App/yyzMouseFinder.zenmod` via `type:"app"` + `cmdPath:"yyzTools\\yyzMouseFinder.exe"`.

A Windows-subsystem program, a **one-shot process**: it does not parse command-line arguments (the `lpCmdLine` of `WinMain` is unused), shows the spotlight as soon as it starts, and exits on its own once the fade-out animation finishes.

## Launch and Exit

| Scenario | Path |
|----------|------|
| Launch | Command palette / dock / global hotkey → `CmdManager::Launch("mouseFinder")` → `Win32Manager::LaunchApp` → `LaunchExeShell` (the `cmdPath` of the `.zenmod` is resolved by `ModuleManager` into an absolute path under `Platform\yyzTools\`) |
| Default hotkey | `Ctrl+2` (in the packaged default config `inno_setup/yyztools.json`, under `shortcutKeys.apps`, with `appId:"mouseFinder"` / `modifier:"2"` (MOD_CONTROL) / `virtualCode:"50"`); hotkeys do not respond in game mode (`WinCmdPlate`'s `WM_HOTKEY` first checks `CONF_GAME_MODE`) |
| Show-on-launch | `FindMyMouseMain()` calls `sonar.RunSonar()` with no "activation gesture" detection; the process existing means the spotlight is visible |
| Self-exit | `StopSonar()` → `SetSonarVisibility(false)` triggers the implicit Opacity animation → batch-completion callback `PostMessage(WM_OPACITY_ANIMATION_COMPLETED)` → `OnOpacityAnimationCompleted()` sees Opacity < 0.01 and does `SW_HIDE` + `PostMessage(WM_CLOSE)` → `DestroyWindow` → `WM_DESTROY` → `OnSonarDestroy()`'s `PostQuitMessage(0)` → message loop ends, process returns |
| Launch again | `CheckExistProcess()` (`Main.cpp`): the named mutex `yyzTools::yyzMouseFinder` already exists → `FindWindow(L"yyzTools::yyzMouseFinder")` sends `PostMessage(WM_QUIT)` to the old instance, and the new instance `return -1` exits immediately |
| Main process exit | `Application::Uninit()` calls `MouseFinder::Instance().Uninit()` (`src/public/MouseFinder.cpp`): `ExistExeProcess(L"yyzMouseFinder.exe")` + `TerminateProcessId` |
| Install/Update | `inno_setup/yyztools.iss` runs `TaskKill('yyzMouseFinder.exe')`; `versions.json`'s `packageKillProcesses.yyztools.killProcesses` also includes it |

## Sonar Mechanism

`SuperSonar<D>` is a CRTP base class (`Shim()` dispatches to the derived class). The only actually-used derived class is `CompositionSpotlight`. Window class name `yyzTools::yyzMouseFinder`, title `Mouse Finder`, `WS_POPUP` + `WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOREDIRECTIONBITMAP` (the last bit is appended by `CompositionSpotlight::GetExtendedStyle()`). `WM_NCHITTEST` always returns `HTTRANSPARENT` to guarantee click-through.

Only one RawInput device is registered (the mouse, `HID_USAGE_GENERIC_MOUSE`, `RIDEV_INPUTSINK`); the keyboard is not monitored.

State machine (`OnMouseTimer()`, `TIMER_ID_TRACK = 100`, `IdlePeriod = 1000ms`):

| `m_sonarStart` | Meaning |
|----------------|---------|
| `NoSonar` (0) | Spotlight stopped |
| `SonarWaitingForMouseMove` (1) | Shown, but the first mouse move has not arrived yet; this state does **not** time out from stillness |
| Other (tick value) | Timestamp of the last move; once still for `IdlePeriod`, `StopSonar()` |

Three end conditions: the mouse moves then stays still for a full second, `RAWMOUSE.usButtonFlags` is non-zero (any button / wheel), or `GetCursorPos` fails (session switched away).

The initial value of `m_sonarPos` is the sentinel `ptNowhere = {LONG_MIN, LONG_MIN}`, used to distinguish "first call" from "actually moved".

## Composition Visual Tree and Animation

```
[root] ContainerVisual (RelativeSizeAdjustment{1,1}, Opacity driven by the implicit animation)
 └ LayerVisual
   ├ [backdrop] SpriteVisual   fills the area, solid color m_backgroundColor
   └ [spotlight] ShapeVisual   AnchorPoint{0.5,0.5}, contains a white CompositionSpriteShape
```

- **Opacity**: `CreateScalarKeyFrameAnimation` + `InsertExpressionKeyFrame(1.0f, L"this.FinalValue")` registered into `CreateImplicitAnimationCollection()`; after that, assigning to `m_root.Opacity()` automatically animates. On show, the target value is `m_finalAlphaNumerator / FinalAlphaDenominator` (`FinalAlphaDenominator` is fixed at 100).
- **Radius**: `CreateExpressionAnimation` bound to `m_circleGeometry`'s `Radius`, with the expression `Lerp(Vector2(r*z, r*z), Vector2(r, r), Root.Opacity * 100 / finalAlpha)` — the higher the opacity, the smaller the radius, producing the "shrink from large to small" focusing effect.
- **Position**: `AfterMoveSonar()` assigns `m_spotlight.Offset(...)` directly, without animation (per the source comment, to improve responsiveness).

## Settings and Defaults

`FindMyMouseSettings` (`MouseFinder.h`); `Main.cpp` uses a **default-constructed instance** with no external configuration source. All six fields are consumed by `ApplySettings`:

| Field | Default | Consumed at |
|-------|---------|-------------|
| `spotlightRadius` | `100` | `m_sonarRadius` |
| `spotlightInitialZoom` | `9` | Radius-expression zoom multiplier |
| `overlayOpacity` | `50` | `m_finalAlphaNumerator`, denominator 100 |
| `animationDurationMs` | `500` | `m_fadeDuration` (changed to 1ms when `<= 0`) |
| `backgroundColor` | ARGB `255,0,0,0` | Dimming background color; actual opacity is decided by the root Opacity |
| `spotlightColor` | ARGB `255,255,255,255` | Spotlight circle fill |

## Conventions and Pitfalls

- Quitting the old instance relies on `PostMessage(hwnd, WM_QUIT, 0, 0)` (`Main.cpp:19`); the loop in `FindMyMouseMain` does **not** need to re-check `msg.message == WM_QUIT`: `GetMessage` returns 0 when it receives `WM_QUIT` (judged by message value; `PostMessage` / `PostThreadMessage` / `PostQuitMessage` all behave the same), and `while (GetMessage(...))` already exits on its own.
- ⚠️ **The single-instance mutex name and the window class name are the same string** `yyzTools::yyzMouseFinder` (`appkey` in `Main.cpp` and `className` in `MouseFinder.cpp`). If you change one you must change the other, otherwise the quit signal cannot find the window.
- ⚠️ **A transparent window must not cover the entire virtual screen**: `StartSonar()`'s `SetWindowPos` deliberately uses `SM_XVIRTUALSCREEN + 1` / `SM_CXVIRTUALSCREEN - 2`; the comment notes this is a workaround for the system glitch where a full-screen transparent window garbles the taskbar's transparency effect. Do not "fix" it into an exact cover.
- ⚠️ **`StopSonar()` does not close the window immediately**; it only animates the opacity to 0. The actual exit depends on the `m_batch.Completed` callback hooked in `SetSonarVisibility`, which posts `WM_OPACITY_ANIMATION_COMPLETED`, and then `OnOpacityAnimationCompleted()` checks `m_root.Opacity() < 0.01f` before doing `SW_HIDE` + `PostMessage(WM_CLOSE)`. The source retains the old comment about the immediate `WM_CLOSE` to explain this change. The animation duration can never reach 0 (`ApplySettings` clamps `animationDurationMs > 0 ? … : 1` to ≥1ms), but if this callback chain breaks the process will never exit.
- ⚠️ **`WM_CREATE` is handled once at each of two levels**: `CompositionSpotlight::WndProc` first calls `OnCompositionCreate()` to build the composition tree, then **always** continues to `BaseWndProc`, which runs `OnSonarCreate()` to register RawInput. The ordering depends on this "derived first, base second" dispatch; if `OnCompositionCreate()` fails it `return -1` and `CreateWindowExW` fails outright.
- ⚠️ **`SetSonarVisibility` is `= delete` in the base class**, a required member of the CRTP contract; a new derived class must provide it, otherwise you get a compile error rather than a runtime surprise.
- ⚠️ **RawInput is only used to detect "whether there is an event", not to parse coordinates**: `OnSonarMouseInput` only looks at `usButtonFlags` to decide whether to stop, and forwards everything else to `OnMouseTimer()`; the position comes from `GetCursorPos`. Therefore `MOUSE_MOVE_ABSOLUTE` packets from VMs / RDP need no special handling.
- Cursor position always uses `GetCursorPos` + `ScreenToClient` (RawInput only gives deltas); `Initialize()` first calls `SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`, and `app.manifest` also declares `PerMonitorV2` — both places are present, so do not change only one.
- Window-class registration is idempotent: `Initialize()` first probes with `GetClassInfoW` and skips `RegisterClassW` if already registered.
- The global `CompositionSpotlight* m_sonar` points to the object on `FindMyMouseMain`'s stack; it is assigned before entering the loop and reset to `nullptr` after the loop, solely for the reentrancy guard "if already running, return directly".
- This project originates from PowerToys' Find My Mouse. The GDI rendering branch (`GdiSonar`/`GdiSpotlight`/`GdiCrosshairs`) from the original implementation, the shake-activation gesture (`shake*` settings and `m_movementHistory`), and unwired config items like `activationMethod`/`excludedApps` **have all been deleted** — this project keeps only the "process exists means show" path and Composition rendering. If you want to add an activation gesture, redesign it; do not resurrect the dead code following the original project.
- `windowsapp.lib` is linked via `#pragma comment(lib, ...)` inside `Main.cpp`; the vcxproj does not list it as an additional dependency.

## Build Output

`OutDir` is `bin64\` (x64; the Win32 config outputs to `bin\`), language standard `stdcpp20`, subsystem `Windows`. The C++/WinRT headers are provided by the Windows SDK; the vcxproj **references no NuGet packages** (the `packages.config` in the directory is a leftover carried over from PowerToys, and `ExtensionTargets` is empty). After building, a PostBuildEvent copies a copy:

```
copy /Y $(SolutionDir)..\bin64\yyzMouseFinder.exe $(SolutionDir)..\bin64\platform\yyzTools\yyzMouseFinder.exe
```

At runtime it uses `bin64/Platform/yyzTools/yyzMouseFinder.exe` (`GetAppDir() + "\\Platform\\yyzTools"` in `src/public/MouseFinder.cpp`, which is also the base directory of `cmdPath` in the `.zenmod`). When packaging, it is recursively pulled into the installer by `inno_setup/yyztools.iss`'s `..\bin64\Platform\*`; incremental updates belong to the `yyztools` sub-package of `versions.json` (`bin64\Platform\yyzTools\`).
