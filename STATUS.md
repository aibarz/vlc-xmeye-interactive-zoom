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
