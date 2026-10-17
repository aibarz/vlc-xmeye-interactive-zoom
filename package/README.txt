VLC XMEye Interactive Zoom - MVP
================================

Target: VLC 3.0.24 x64 on Windows.

Install
-------
1. Close VLC.
2. Run INSTALL-ZOOM.cmd.
3. Start VLC with START-VLC-WITH-ZOOM.cmd.

If VLC is installed elsewhere:
  INSTALL-ZOOM.cmd "D:\ruta\a\VLC"
  START-VLC-WITH-ZOOM.cmd "D:\ruta\a\VLC"

Controls
--------
- Drag inside the large ROI map to move the visible center live.
- Mouse wheel over the ROI map changes zoom in 25% steps.
- Slider gives continuous 100-800% control.
- Center returns X/Y to 50/50.
- 100% resets to the full frame.
- Detect W/H reads the active decoded VLC frame once.
- Source W/H remain editable as a manual override.
- Save/Restore persists zoom, center, source size and window position.
- Always on top is intended for dual-monitor work.

Implementation
--------------
The module changes VLC crop margins and keeps autoscale enabled.
It does not transcode media and does not change XMEye demux DLLs.

Plugin:
  plugins\control\libxmeyezoom_plugin.dll
Shortcut:
  xmeyezoom
CLI:
  vlc.exe --extraintf=xmeyezoom

Project:
  https://github.com/aibarz/vlc-xmeye-interactive-zoom
