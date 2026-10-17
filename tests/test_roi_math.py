def clamp(v, lo, hi):
    return max(lo, min(hi, v))


def center_limits(zoom):
    half = (5000 + zoom // 2) // zoom
    half = clamp(half, 0, 50)
    return half, 100 - half


def roi(sw, sh, zoom, cx, cy):
    lo, hi = center_limits(zoom)
    cx = clamp(cx, lo, hi)
    cy = clamp(cy, lo, hi)
    vw = (sw * 100 + zoom // 2) // zoom
    vh = (sh * 100 + zoom // 2) // zoom
    vw = clamp(vw, 2, sw)
    vh = clamp(vh, 2, sh)
    left = clamp((cx * sw) // 100 - vw // 2, 0, sw - vw)
    top = clamp((cy * sh) // 100 - vh // 2, 0, sh - vh)
    right = sw - vw - left
    bottom = sh - vh - top
    return left, top, right, bottom, vw, vh, cx, cy


def test_full_frame():
    assert roi(2592, 1944, 100, 50, 50)[:6] == (0, 0, 0, 0, 2592, 1944)


def test_center_2x():
    left, top, right, bottom, vw, vh, cx, cy = roi(2592, 1944, 200, 50, 50)
    assert (vw, vh) == (1296, 972)
    assert left == right == 648
    assert top == bottom == 486
    assert (cx, cy) == (50, 50)


def test_physical_clamp_2x():
    left, top, right, bottom, _, _, cx, cy = roi(2592, 1944, 200, 0, 100)
    assert (cx, cy) == (25, 75)
    assert left == 0
    assert bottom == 0


def test_8x_limits():
    lo, hi = center_limits(800)
    assert (lo, hi) == (6, 94)
    left, top, right, bottom, vw, vh, cx, cy = roi(2592, 1944, 800, 0, 100)
    assert (vw, vh) == (324, 243)
    assert cx == 6 and cy == 94
    assert left == 0 or left <= 1
    assert bottom == 0 or bottom <= 1


def test_720p():
    left, top, right, bottom, vw, vh, _, _ = roi(1280, 720, 400, 50, 50)
    assert (vw, vh) == (320, 180)
    assert (left, right) == (480, 480)
    assert (top, bottom) == (270, 270)


if __name__ == "__main__":
    tests = [v for k, v in globals().items() if k.startswith("test_") and callable(v)]
    for t in tests:
        t()
    print(f"OK: {len(tests)} ROI tests")
