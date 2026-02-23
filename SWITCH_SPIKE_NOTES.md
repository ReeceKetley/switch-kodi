# Kodi 18.9 Switch Spike Notes

## Scope
- Target: Nintendo Switch (`libnx`/devkitPro toolchain).
- Build dir: `build-switch-spike`.
- Strategy: targeted blocker removal before full `kodi` link.

## Current Status
- `pcre` target builds successfully.
- `tinyxml` target builds successfully.
- `platform_linux_network` target has been removed from Switch build graph.
- Full `make -j4 kodi` now completes and links `kodi-switch` in `build-switch-spike`.

## Confirmed Milestones
- Internal `RapidJSON` build path fixed to install headers into build include path.
- Switch portability shims and guards added for:
  - `uname` usage in system info.
  - `sys/uio.h` (`iovec`) where needed.
  - `sys/mman.h` (`mmap`/`munmap`) minimal compatibility.
  - `statvfs` path on Switch.
  - Linux-only networking/timezone/time APIs guarded or stubbed for Switch.
- Overlay renderer name collision fixed (`quad` -> `verts` in overlay quad struct usage).
- FreeType include path wired for Switch Teletext compilation.
- `SA_RESTART` usage in `xbmc/platform/posix/main.cpp` guarded for platforms where it is missing.
- Switch platform CMake now clears `CMAKE_DL_LIBS` to avoid invalid `-ldl` link on libnx.
- Switch `FindIconv.cmake` path no longer injects explicit `libc.a` into link line.
- Added broad Switch compatibility shim (`xbmc/utils/SwitchCompat.cpp`) covering:
  - legacy FFmpeg symbols expected by Kodi 18 (`avcodec_encode_audio2`, `swr_alloc_set_opts`, channel layout helpers, qp table helper, etc.),
  - libc/posix gaps (`fchown`, `geteuid`, iconv shim),
  - cpluff API stubs used by addon manager paths,
  - emu environment no-op hooks.
- Added Switch network backend (`xbmc/network/NetworkSwitch.*`) and routed `CNetwork` alias to it on Switch.
- Added Switch window system factory stub (`xbmc/windowing/WinSystemSwitch.cpp`) so `CreateWinSystem()` resolves.
- Additional source guards for Switch:
  - `popen/pclose` and shell-command paths,
  - optical-only directory creation paths,
  - CPU/GPU temp probes and Linux resource counters where unsupported.
- Link workaround for final binary:
  - `-Wl,-z,notext` added in Switch platform CMake to permit final executable link with current GLES stack.
  - `-Wl,-z,nopack-relative-relocs` added to force `RELA` relocations (disable `RELR`) for better Ryujinx homebrew loader compatibility.

## Recent Verification Commands
- `make help | grep -Ei '(^| )pcre($| )|(^| )tinyxml($| )'`
- `make -j4 pcre tinyxml`
- `make help | grep -E 'platform_linux_network|linuxnetwork|kodi$'`

## Next Steps
1. Replace shimmed/stubbed subsystems with real Switch implementations incrementally:
   - UUID/crossguid backend finalization,
   - iconv implementation,
   - addon/cpluff strategy.
2. Runtime smoke-test `kodi-switch` in emulator and on hardware; identify first startup/runtime crash points.
3. Reduce reliance on permissive linker workaround (`-z notext`) by moving to a Switch-safe render/GLES stack configuration.
4. Validate startup progression after `RELR` removal and capture the next concrete runtime blocker (expected in windowing/GLES init path).

## Session Handoff (2026-02-22)

### Key Finding: Renderer Pipeline Is Alive
- Verified with a pre-window-manager heartbeat path that executes:
  - `BeginRender() -> Clear(color) -> EndRender() -> Flip()`
  - before `WindowManager::Process()`.
- User confirmed visible solid-color output in Eden (`red` observed, FPS active).
- Conclusion: current black-screen/hang is not "no renderer output"; blocker is in GUI/update path before normal `CApplication::Render()`.

### Current Runtime Blocker
- Main loop reaches:
  - `LOOPDBG tick=1 FrameMove begin`
  - `SWITCH_FM: WM.Process begin`
  - `SWITCH_WMP: active DoProcess begin ptr=...`
- Then stalls during Home window `DoProcess` path (pre-render stage).

### Known-Good Build Shell
- Use this shell wrapper (required for consistent tools and PATH):
  - `D:\SwitchDev\switch-shell.ps1`
- Do not build from plain PowerShell without this wrapper.

### Canonical Build + Pack + Deploy Commands
- Build:
  - `& 'D:\SwitchDev\switch-shell.ps1' "cd '/d/SwitchDev/ports/kodi-18.9/build-switch-spike' && make -j4 kodi"`
- Pack:
  - `& 'D:\SwitchDev\switch-shell.ps1' "cd '/d/SwitchDev/ports/kodi-18.9/build-switch-spike' && /opt/devkitpro/tools/bin/elf2nro kodi-switch kodi-switch-slim-debug.nro --nacp=kodi-switch.nacp"`
- Deploy to Eden SDMC mirror:
  - `copy D:\SwitchDev\ports\kodi-18.9\build-switch-spike\kodi-switch-slim-debug.nro C:\Users\recox\AppData\Roaming\eden\sdmc\switch\kodi-switch-slim-debug\kodi-switch-slim-debug.nro`

### Trace Config Path
- Active config file:
  - `C:\Users\recox\AppData\Roaming\eden\sdmc\switch\kodi-switch-slim-debug\trace.cfg`

### Trace Config Presets
- Renderer heartbeat isolation (bypasses WM.Process):
  - `pre_wm_flash_test=1`
  - `pre_wm_flash_only=1`
- Older skin/alloc skip baseline used during debugging:
  - `skip_alloc_id=29999`
  - `skip_skin_color=1`
  - `skip_skin_includes=1`
  - `skip_skin_fonts=1`
  - `skip_skin_strings=1`
  - `skip_custom_windows=1`
  - `skip_wm_initialize=1`
  - `skip_gui_audio_load=1`
- Keep:
  - `trace=1`

### Debug Instrumentation Added
- `xbmc/Application.cpp`
  - `SWITCH_FM` and `SWITCH_RENDER` logging.
  - Pre-WM renderer heartbeat test via `pre_wm_flash_test` / `pre_wm_flash_only`.
- `xbmc/guilib/GUIWindowManager.cpp`
  - `SWITCH_WMP` tracing around active/dialog `DoProcess`.
- `xbmc/guilib/GUIControlGroup.cpp`
  - `SWITCH_GRP_PROC` per-child process tracing for `WINDOW_HOME` and `WINDOW_DIALOG_VOLUME_BAR`.

## Session Update (2026-02-23)

### Path Policy Simplification
- Reverted to deterministic app root on Switch:
  - `sdmc:/switch/kodi`
- Rationale:
  - auto-root detection introduced instability and wrong-root startup (`sdmc:/switch/kodi` vs app folder mismatch in some runs).
  - fixed-root is simpler and avoids losing time on path heuristics.
- File:
  - `xbmc/settings/SettingsComponent.cpp`

### Font Hang Root Cause + Fix
- Symptom:
  - with `skip_skin_fonts=0`, startup stalled during first font load.
- Instrumentation added:
  - `SWITCH_FONT:` logs in `xbmc/guilib/GUIFontManager.cpp`.
  - `trace.cfg` options:
    - `trace_fonts=1`
    - `skip_font_file=<ttf>`
- Root cause:
  - scaled font params became invalid before TTF load (`newSize <= 0`, `aspect = nan`).
- Fix:
  - sanitize/clamp non-finite or non-positive scaled values before `CGUIFontTTF::Load`.
- Result:
  - no longer dependent on `skip_skin_fonts=1` for this blocker.

### Current Reference Build
- Latest build with font fix:
  - `kodi-switch-fontfix.nro`
- Deployed to:
  - `C:\Users\recox\AppData\Roaming\eden\sdmc\switch\kodi\kodi-switch-fontfix.nro`

## Significant Step Forward (2026-02-23 late)

### Render Crash Root Cause (Found + Fixed)
- Symptom:
  - crash during dialog render (`WINDOW_DIALOG_PROGRESS` / id `10101`) while drawing radio control icon (`icons/logo.png`).
- Exact fault:
  - texture segment geometry contained non-finite coordinates:
    - `SWITCH_TEX_RENDER2: seg-begin ... ltrb=(849,nan,897,nan)`
  - crash happened downstream in render path with invalid vertex values.
- Root cause:
  - `CGUITextureBase::CalculateSize()` could produce non-finite geometry in early startup state.
- Fix:
  - `xbmc/guilib/GUITexture.cpp`
  - guard/sanitize invalid `pixelRatio` and `fOutputFrameRatio`.
  - detect non-finite computed geometry and fallback to safe values.
- Result:
  - dialog `10101` now completes render without crash:
    - `SWITCH_WM_RENDER: dialog DoRender done id=10101`
  - user-visible progress moved from immediate crash to partial GUI render (blue screen/progress UI shown).

### Texture Manager Trace Path Fix
- `xbmc/guilib/TextureManager.cpp` now reads:
  - `sdmc:/switch/kodi/trace.cfg`
  - (in addition to older fallback trace paths)
- This made `trace_tm_all=1` effective in the active runtime layout.

### Current Observations After Fix
- Texture manager confirms many assets are loadable from skin media paths, including:
  - `special://home/addons/skin.estuary/media/icons/logo.png`
  - `special://home/addons/skin.estuary/media/dialogs/close.png`
  - `special://home/addons/skin.estuary/media/progress/texturebg_white.png`
- Remaining runtime issue is no longer the NaN crash; continued work should focus on why some controls still render as empty/0-frame in specific phases.

## Session Update (2026-02-23 night, continued)

### New Render-Path Isolation Results
- Added deeper render probes:
  - `CGUIWindowManager::Render()` entry/exit checkpoints.
  - `CGUIControlGroupList::Render()` per-child checkpoints.
  - `CGUIButtonControl::Render()` phase checkpoints (focus/nofocus/text/base).

- Confirmed path progression:
  - now reaches `SWITCH_WM_RENDER: active DoRender begin id=10000`.
  - first hard block moved from group-list setup into nested control rendering.

### Fixed/Guarded During This Round
- `skip_process_id=0` / `skip_alloc_id=0` / `trace_button_alloc_id=0` no longer accidentally target control id `0`.
  - files:
    - `xbmc/guilib/GUIControlGroup.cpp`
    - `xbmc/guilib/GUIButtonControl.cpp`
- Added finite guards for group-list origin/size math:
  - file:
    - `xbmc/guilib/GUIControlGroupList.cpp`
- Added finite guards for button width/height propagation:
  - file:
    - `xbmc/guilib/GUIButtonControl.cpp`

### Current Precise Bottleneck
- With current probes, first frame render now gets through button `id=5500` only when button text draw is bypassed:
  - `skip_button_text=1` (new trace cfg option in `GUIButtonControl.cpp`)
- After bypass, render advances to next blocker:
  - `SWITCH_GRP_RENDER: child begin parent=10000 idx=1 id=9000 type=31`
  - type `31` = `GUICONTAINER_FIXEDLIST`.

### Takeaway
- Root cause is now narrowed away from generic GL present/swap.
- Active issue is inside home-window UI control rendering sequence, currently centered on text/container render path (post-button stage).

## Session Update (2026-02-23, logo/color root-cause)

### Resolved: Invisible Kodi Logo
- Symptom:
  - top-left `icons/logo.png` did not appear, while other UI elements rendered.
- Investigation:
  - forced `logo.png` to `power.png` proved texture draw path worked.
  - render logs showed logo quad drawn with alpha 0 color (`... color=(0,0,11,0)`).
- Root cause:
  - `CGUIColorManager::GetColor()` fallback used permissive hex parse.
  - unknown token `button_focus` was partially parsed as hex prefix (`b` => `0x0000000B`), producing transparent tint.
- Fix:
  - `xbmc/guilib/GUIColorManager.cpp`
  - replaced permissive fallback with strict full-string hex parse (`strtoul` + end-pointer validation).
  - invalid/non-hex names now return `0` rather than accidental partial values.
- Result:
  - restored original `logo.png`; Kodi logo now renders correctly.
  - main home UI text and layout are readable and visible in Eden.
