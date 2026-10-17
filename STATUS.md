# STATUS

## 2026-10-17

### Project created

Repository created as a separate project so zoom development cannot destabilize the validated XMEye demux/AVI integration.

### Known-good reference behavior

Previous native Qt prototype already demonstrated:

- Zoom 100–800%.
- Dragging a ROI/center on a control canvas.
- Live `crop` updates on active VLC vouts.
- Autoscale after crop.
- Center X/Y clamping based on zoom.
- OSD zoom percentage.
- Save/restore through QSettings.
- Manual source W/H fields.

### Current technical direction

Start with the crop/autoscale model because it is already proven inside VLC. Improve it by auto-detecting source dimensions and then add direct interaction on the playing video.

### Next task

1. Extract the zoom logic from the previous combined XMEye Qt patch.
2. Build the smallest standalone zoom implementation against VLC 3.0.24.
3. Validate live zoom on ASUS before adding more UI complexity.

## 2026-10-17 — MVP implementation started

- Standalone Win32 interface module added as `libxmeyezoom_plugin.dll` (`--extraintf=xmeyezoom`).
- Zoom engine uses VLC `crop` border margins + `autoscale`; demux/decoder code remains untouched.
- Floating ROI map supports live drag-to-position and mouse-wheel zoom.
- Trackbar range 100–800%; physical X/Y limits are enforced as zoom changes.
- Source W/H can be edited manually and the plugin attempts one-shot auto-detection from the active vout snapshot.
- Zoom percentage is shown through VLC OSD while zoom changes.
- State persistence uses `%APPDATA%\\vlc\\xmeye-zoom.ini`.
- Installer, launcher and removal scripts prepared under `package/`.
- ROI math tests cover 2592×1944 and 1280×720; current result: 5/5 passing.
- Windows x64 CI build is wired against VLC 3.0.24. Runtime validation on ASUS is still required before merging `dev/interactive-zoom` to `main`.

## 2026-10-17 — interface-only runtime check

- CI artifact from commit `c994f07` downloaded successfully.
- `libxmeyezoom_plugin.dll` installed into the portable VLC 3.0.24 runtime under `plugins/control/` and plugin cache regenerated.
- VLC started with `-I dummy --extraintf=xmeyezoom -vvv` and explicitly loaded interface module `xmeye_zoom_win32`.
- This validates module discovery/loading and the standalone UI path on the T520 without loading video.
- ASUS remains the functional validation target for live 5 MP zoom/pan behavior.
- Packaging scripts now auto-detect common VLC locations before falling back to an explicit path argument.
