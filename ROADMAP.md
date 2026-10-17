# ROADMAP

## Phase 0 — isolate zoom from demux

- Keep `vlc-xmeye-demux` frozen as validated baseline.
- Reuse only the proven zoom ideas from previous UI prototypes.
- Establish independent build/test workflow.

## Phase 1 — zoom engine MVP

- Implement zoom through VLC crop/autoscale without transcoding.
- Correct ROI math for 100–800% zoom.
- Clamp center X/Y to physically valid limits.
- Auto-detect source dimensions when VLC exposes them; retain manual override.

## Phase 2 — interactive control

- Drag ROI on a visual canvas.
- Live zoom slider/spinbox.
- Center/reset controls.
- Mouse wheel support on the control canvas.
- OSD percentage while zoom changes.

## Phase 3 — direct-on-video interaction

- Optional click/drag directly over the playing video.
- Wheel zoom around cursor position.
- Keyboard shortcuts for center, zoom in/out and reset.
- Do not steal normal VLC input when interactive mode is disabled.

## Phase 4 — persistence and presets

- Save/restore last state.
- Named presets per camera/source resolution.
- Optional per-file/per-camera remembered ROI.

## Phase 5 — UI variants

- Floating Win32 panel for dual-monitor workflows.
- Native Qt panel integrated into VLC extended controls.
- Keep a lightweight Lua controller only where it adds value.

## Phase 6 — validation

- Test 1280x720, 2592x1944 and other source sizes.
- Verify no regression to XMEye playback.
- Measure CPU/GPU impact on ASUS and T520.
- Package reproducible VLC 3.0.24 x64 build/install artifacts.
