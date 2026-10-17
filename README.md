# VLC XMEye Interactive Zoom

Interactive, configurable zoom for VLC 3.0.24 x64, aimed at surveillance/XMEye playback but kept independent from the XMEye demux layer.

## Goal

Provide a live zoom that can be moved and changed while video is playing, without transcoding and without modifying the validated XMEye demux baseline.

Core controls:

- Zoom: 100% to 800%.
- Center X/Y with physical limits derived from zoom.
- Drag-to-position ROI.
- Live apply while playback continues.
- Source dimensions: automatic when possible, manual override when needed.
- Save/restore position and zoom.
- OSD zoom readout.
- Optional floating control panel for dual-monitor use.

## Baseline

The XMEye demux/AVI integration is maintained separately in `aibarz/vlc-xmeye-demux` and is considered validated for VLC 3.0.24 x64. This repository must not alter that demux baseline.

Existing experimental zoom work is available in the earlier VLC UI branches and is treated only as a reference implementation. The current native Qt prototype uses VLC's `crop` variable to calculate a rectangular ROI and lets VLC autoscale that ROI back to the video output.

## Initial control model

`zoom_percent`, `center_x_percent`, `center_y_percent`, `source_width`, `source_height`, `enabled`.

At zoom factor `z`, visible size is approximately `source / z`; center coordinates are clamped so the ROI never leaves the source frame.

## Target

- VLC 3.0.24 x64 on Windows first.
- ASUS is the performance/validation machine.
- HP T520 remains useful for low-load UI tests, not as the performance reference for heavy 5 MP HEVC.

See `ROADMAP.md`, `STATUS.md` and `docs/ARCHITECTURE.md`.

## Current MVP

The first standalone implementation is a Win32 VLC interface plugin:

```text
plugins\\control\\libxmeyezoom_plugin.dll
vlc.exe --extraintf=xmeyezoom
```

Interaction is intentionally separated from the XMEye demux. The floating controller provides a visual ROI map: drag the ROI to pan, use the mouse wheel or slider to change zoom, auto-detect source W/H when possible, or type W/H manually as an override. The control window can remain always-on-top for dual-monitor work.

Direct mouse interaction on the actual VLC video window remains the next layer; it will only be enabled explicitly so normal VLC mouse behavior is preserved when interactive mode is off.
