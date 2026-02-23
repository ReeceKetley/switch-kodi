# Kodi Switch Session Handoff (2026-02-23)

## Canonical Workspace
- Repo: `D:\SwitchDev\ports\kodi-18.9`
- Build dir: `D:\SwitchDev\ports\kodi-18.9\build-switch-spike`
- Use this shell for build tools/PATH: `D:\SwitchDev\switch-shell.ps1`

## Canonical Runtime Layout
- Keep app root fixed to `sdmc:/switch/kodi`.
- Eden mirror path:
  - `C:\Users\recox\AppData\Roaming\eden\sdmc\switch\kodi`
- Trace config:
  - `C:\Users\recox\AppData\Roaming\eden\sdmc\switch\kodi\trace.cfg`
- Kodi log:
  - `C:\Users\recox\AppData\Roaming\eden\sdmc\switch\kodi\temp\kodi.log`
- Eden log:
  - `C:\Users\recox\AppData\Roaming\eden\log\eden_log.txt`

## Known Important Decisions
- Do not auto-detect random app roots right now.
- Force deterministic Switch root in code:
  - `xbmc/settings/SettingsComponent.cpp` uses `sdmc:/switch/kodi` for `appPath`, `appBinPath`, `userHome`.
- This avoids unstable startup caused by bad root selection (`sdmc:/`, `sdmc:/switch/kodi-switch-*`, etc).

## Core Fixes Already Landed
- Switch file/path compatibility layer added (`xbmc/filesystem/SwitchFile.*`) and used to stabilize local IO semantics.
- SQLite open behavior hardened in `xbmc/dbwrappers/sqlitedataset.cpp`:
  - recursive parent dir creation
  - fallback candidates and normalized path attempts
  - better diagnostics
- FFmpeg image decode path hardened in `xbmc/guilib/FFmpegImage.cpp`:
  - fixed custom seek behavior
  - Switch direct decode path to avoid custom AVIO hangs
- Renderer heartbeat test added (`Application.cpp`) to prove render pipeline works before WM flow.
- Font hang root cause found and fixed in `xbmc/guilib/GUIFontManager.cpp`:
  - during first font load, scaled values became invalid (`newSize<=0`, `aspect=nan`)
  - added Switch guard to clamp/sanitize invalid scaled values before `CGUIFontTTF::Load`
  - added Switch font tracing and per-file skip option in `trace.cfg`

## Current Status
- `skip_skin_fonts=1` is no longer required after font fix.
- Repro showed hang at first font load:
  - `SWITCH_FONT: pFontFile->Load begin ... notosans-regular.ttf`
  - with invalid scaled size/aspect.
- New guard patch resolves that specific blocker.

## Active Debug Controls (`trace.cfg`)
- Global:
  - `trace=1`
- Font diagnostics:
  - `trace_fonts=1`
  - `skip_font_file=` (empty means no file skipped)
- Keep these off unless needed:
  - `pre_wm_flash_test=0`
  - `pre_wm_flash_only=0`
  - `skip_skin_fonts=0`
  - `skip_process_window=0`
  - `skip_process_id=0`

## Build, Pack, Deploy
```powershell
& 'D:\SwitchDev\switch-shell.ps1' -Command "cd /d/SwitchDev/ports/kodi-18.9/build-switch-spike && make -j4"
& 'D:\SwitchDev\switch-shell.ps1' -Command "cd /d/SwitchDev/ports/kodi-18.9/build-switch-spike && elf2nro kodi-switch kodi-switch.nro --nacp=kodi-switch.nacp"
Copy-Item -Force 'D:\SwitchDev\ports\kodi-18.9\build-switch-spike\kodi-switch.nro' 'C:\Users\recox\AppData\Roaming\eden\sdmc\switch\kodi\kodi-switch.nro'
```

## Latest Session Test Binaries
- Path-root deterministic build:
  - `C:\Users\recox\AppData\Roaming\eden\sdmc\switch\kodi\kodi-switch-kodiroot.nro`
- Font tracing build:
  - `C:\Users\recox\AppData\Roaming\eden\sdmc\switch\kodi\kodi-switch-fonttrace.nro`
- Font fix build (latest):
  - `C:\Users\recox\AppData\Roaming\eden\sdmc\switch\kodi\kodi-switch-fontfix.nro`

## Quick Repro Loop
1. Run NRO with Eden/Eden-cli.
2. Let it run until hang/exit.
3. Ensure stale Eden processes are closed before next run.
4. Check:
   - `kodi.log` for `FATAL`, `ERROR`, `SWITCH_FONT:`, `SWITCH_WM*`
   - `eden_log.txt` for FS/service behavior

## Highest-Value Next Step
- Validate post-font-fix progression with `kodi-switch-fontfix.nro` and `skip_skin_fonts=0`.
- If still hung, use `SWITCH_FONT:` and `SWITCH_WM*` lines to isolate next exact stage instead of reintroducing broad skips.

## Major New Milestone (2026-02-23)
- Crash root cause in render path was identified and fixed:
  - `CGUITextureBase::CalculateSize()` produced non-finite coordinates for a dialog radio icon draw (`icons/logo.png`), causing crash during segment render.
  - Added non-finite guards and safe fallback geometry in `xbmc/guilib/GUITexture.cpp`.
- Runtime moved forward substantially:
  - no immediate crash at dialog `10101`.
  - UI now reaches visible partial screen in Eden (blue background/progress elements), confirming this blocker is removed.
- TextureManager trace path updated to active layout:
  - `xbmc/guilib/TextureManager.cpp` now checks `sdmc:/switch/kodi/trace.cfg`.
  - `trace_tm_all=1` now works reliably for current deployment root.

## Current Bottleneck Update (2026-02-23, later)
- Rendering no longer stalls at the earlier radio-button path.
- New precise stall chain found in first `WINDOW_HOME` render:
  1. `CGUIWindowManager::Render()` enters and reaches `active DoRender` (`id=10000`).
  2. In group-list render, first hard stall was at button `id=5500` (type `GUICONTROL_BUTTON`).
  3. This was narrowed to button text path (`RenderText`) and made configurable for testing.
  4. With `skip_button_text=1`, render progresses past `id=5500` and reaches next blocker:
     - child `id=9000`, type `GUICONTAINER_FIXEDLIST`.

- Related guard/fixes added:
  - `GUIControlGroup.cpp` / `GUIButtonControl.cpp`:
    - `skip_*_id=0` in `trace.cfg` no longer accidentally matches control ID `0`.
  - `GUIButtonControl.cpp`:
    - non-finite width/height sanitization in button sizing path.
    - switch cfg lookup now includes `sdmc:/switch/kodi/trace.cfg`.
    - optional render probe: `skip_button_text=1` to bypass text draw for isolation.
  - `GUIControlGroupList.cpp`:
    - non-finite origin/size guards and detailed render tracing for first calls.

- Current debug `trace.cfg` essentials:
  - `trace_render_steps=1`
  - `trace_render_window=10000`
  - `skip_button_text=1` (diagnostic only)

## Latest Milestone (2026-02-23, 22:19)
- Main-menu UI is now functionally visible and readable in Eden:
  - left nav labels render correctly (`Movies`, `TV shows`, `Music`, etc.).
  - center text is readable and no longer glyph-corrupted.
  - top-left Kodi logo now renders correctly.
- Root cause for missing top-left logo was identified in color parsing:
  - unknown color names were being partially parsed as hex.
  - example: `button_focus` fell through to `%x` parsing and became `0x0000000B` (alpha 0), making the texture fully transparent.
- Fix landed in:
  - `xbmc/guilib/GUIColorManager.cpp`
  - `GetColor()` now only accepts fully valid hex strings via `strtoul(..., base 16)` with full-string validation.
  - non-hex/partial strings now return `0` instead of accidental partial parse values.
- Verification:
  - temporary texture swap test (`logo.png` <- `power.png`) confirmed draw path was active.
  - after parser fix + restoring original `logo.png`, proper logo appears.

## What This Repo Is Right Now
- A focused Switch bring-up branch for Kodi 18.9:
  - filesystem/path compatibility for `sdmc:/switch/kodi`,
  - sqlite open/create hardening,
  - font/render non-finite guards,
  - UTF conversion fixes for readable text,
  - color parser hardening that fixed invisible logo tinting.
- This is still a spike/porting branch, not upstream-ready cleanup.

## Current Runtime State
- Home screen is now up with usable text and core skin layout.
- Remaining work is refinement/stability, including:
  - residual low-FPS/hang behavior in some paths,
  - additional skin/image correctness checks,
  - input/polish and emulator/hardware validation.
