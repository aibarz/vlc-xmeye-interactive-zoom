# Architecture

## Separation of concerns

The zoom project is independent from XMEye parsing. It receives an already decoded VLC video output and controls the visible source rectangle.

`input/demux -> decoder -> video output -> zoom ROI/crop -> autoscale/display`

## State

```text
enabled
zoom_percent      100..800
center_x_percent  0..100, dynamically clamped
center_y_percent  0..100, dynamically clamped
source_width
source_height
```

For zoom factor `z = zoom_percent / 100`:

```text
visible_width  = source_width  / z
visible_height = source_height / z
left = center_x * source_width / 100 - visible_width / 2
top  = center_y * source_height / 100 - visible_height / 2
```

`left/top` are clamped to the source frame. VLC receives crop margins as `left+top+right+bottom`, then `autoscale=true` fills the output window.

## Interaction layers

1. Engine: ROI math and VLC variable updates.
2. Control model: validated state, presets and persistence.
3. UI: native Qt and/or floating Win32 controller.
4. Direct video input: optional mouse/wheel hooks, isolated so normal VLC controls remain intact when disabled.

## Constraints

- No re-encoding.
- No temporary video files.
- Zoom must update during playback.
- Demux DLLs are out of scope and must remain unchanged.
- Heavy 5 MP HEVC performance is validated on ASUS, not inferred from the T520.
