VLC XMEye Desk - Zoom + Extensions
==================================

Target: VLC 3.0.24 x64 on Windows.

Purpose
-------
Run the validated interactive zoom together with the floating XMEye extensions panel so the video can stay on one display and the controls on another.

Install
-------
1. Close VLC.
2. Run INSTALL-XMEYE-DESK.cmd.
3. Start VLC with START-VLC-XMEYE-DESK.cmd.

If VLC is installed elsewhere:
  INSTALL-XMEYE-DESK.cmd "D:\ruta\a\VLC"
  START-VLC-XMEYE-DESK.cmd "D:\ruta\a\VLC"

Installed control modules
-------------------------
- libxmeyezoom_plugin.dll      Interactive ROI zoom/pan.
- libxmeyecontrol_plugin.dll   Floating XMEye extension controls.

Installed video filters
-----------------------
- libxmeyegate_plugin.dll      Static gate.
- libxmeyedeblock_plugin.dll   Deblock.
- libxmeyedeconv_plugin.dll    Deconvolution/detail correction.
- libxmeyegrayguard_plugin.dll Gray-frame guard.
- libxmeyefocus_plugin.dll     Focus/detail correction.

Launch
------
START-VLC-XMEYE-DESK.cmd starts both interfaces with:
  --extraintf=xmeyezoom:xmeyecontrol

The two floating windows can be moved independently to a second monitor. Their positions/settings are persisted by the modules.

Safety
------
This package does not modify the validated XMEye demux/AVI integration. Existing same-name plugins are backed up as .previous.bak before replacement and REMOVE-XMEYE-DESK.cmd restores those backups.
