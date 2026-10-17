# Legacy zoom references

These files are snapshots of earlier experimental XMEye/VLC UI work and are **not** the canonical implementation for this repository.

They are kept only to avoid losing proven ideas while the zoom code is extracted into a standalone design.

- `legacy-nativeqt-combined.patch` — combined VLC Qt patch containing the native Zoom tab plus unrelated XMEye adjustment controls.
- `legacy-floating-control.c` — Win32 floating controller prototype used for dual-monitor testing.
- `legacy-xmeye-tools.lua` — Lua controller prototype.

Relevant earlier commits:

- Native Qt UI branch: `f7f646b` / direct source reference `a1366c5`.
- Floating controller branch: `4beb15f`.
- Lua controller branch: `58e8e7d`.

The first development task is to extract only the zoom engine/UI behavior needed here and discard the unrelated filter controls.
