/*****************************************************************************
 * xmeye_zoom_win32.c: interactive configurable zoom for VLC 3.x / Windows
 *****************************************************************************
 * Original concept and functional design: Álvaro Ibarz
 * https://github.com/aibarz
 *
 * Zoom is implemented by changing VLC's crop margins and keeping autoscale on.
 * No decoding, demuxing or transcoding is performed by this module.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define VLC_MODULE_LICENSE VLC_LICENSE_GPL_2_PLUS
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_playlist.h>
#include <vlc_input.h>
#include <vlc_vout.h>
#include <vlc_vout_osd.h>
#include <vlc_picture.h>

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PREVIEW_LEFT   24
#define PREVIEW_TOP    62
#define PREVIEW_RIGHT  756
#define PREVIEW_BOTTOM 382

#define ZOOM_MIN 100
#define ZOOM_MAX 800
#define ZOOM_STEP 25

enum
{
    ID_ZOOM_ACTIVE = 1001,
    ID_TOPMOST,
    ID_ZOOM_TRACK,
    ID_ZOOM_EDIT,
    ID_CENTER_X,
    ID_CENTER_Y,
    ID_SOURCE_W,
    ID_SOURCE_H,
    ID_SOURCE_AUTO,
    ID_CENTER,
    ID_RESET,
    ID_SAVE,
    ID_RESTORE,
    ID_STATUS,
    ID_CREDIT
};

typedef struct intf_sys_t
{
    HWND window;
    HANDLE ready;
    vlc_thread_t thread;

    HWND zoom_track;
    HWND zoom_edit;
    HWND center_x;
    HWND center_y;
    HWND source_w;
    HWND source_h;
    HWND status;
    HWND credit;

    HFONT title_font;
    HFONT credit_font;

    bool dragging;
    bool applying;
    bool detected_once;

    int zoom;
    int cx;
    int cy;
    int source_width;
    int source_height;
} intf_sys_t;

static int Open(vlc_object_t *);
static void Close(vlc_object_t *);

static int clamp_int(int value, int lo, int hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

static void preview_rect(RECT *r)
{
    r->left = PREVIEW_LEFT;
    r->top = PREVIEW_TOP;
    r->right = PREVIEW_RIGHT;
    r->bottom = PREVIEW_BOTTOM;
}

static void set_default_font(HWND h)
{
    if (h != NULL)
        SendMessageW(h, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
}

static HWND make_static(HWND parent, int x, int y, int w, int h,
                        const wchar_t *text, int id)
{
    HWND c = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE,
                             x, y, w, h, parent, (HMENU)(INT_PTR)id,
                             GetModuleHandleW(NULL), NULL);
    set_default_font(c);
    return c;
}

static HWND make_button(HWND parent, int x, int y, int w, int h,
                        const wchar_t *text, int id, DWORD style)
{
    HWND c = CreateWindowExW(0, L"BUTTON", text,
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | style,
                             x, y, w, h, parent, (HMENU)(INT_PTR)id,
                             GetModuleHandleW(NULL), NULL);
    set_default_font(c);
    return c;
}

static HWND make_edit(HWND parent, int x, int y, int w, int h,
                      const wchar_t *text, int id)
{
    HWND c = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", text,
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                             ES_NUMBER | ES_CENTER | ES_AUTOHSCROLL,
                             x, y, w, h, parent, (HMENU)(INT_PTR)id,
                             GetModuleHandleW(NULL), NULL);
    set_default_font(c);
    return c;
}

static void set_edit_int(HWND edit, int value)
{
    wchar_t buf[32];
    _snwprintf(buf, 31, L"%d", value);
    buf[31] = 0;
    SetWindowTextW(edit, buf);
}

static int get_edit_int(HWND edit, int fallback, int lo, int hi)
{
    wchar_t buf[64];
    if (edit == NULL || GetWindowTextW(edit, buf, 64) <= 0)
        return fallback;
    return clamp_int(_wtoi(buf), lo, hi);
}

static void settings_path(wchar_t *path, size_t count)
{
    wchar_t appdata[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH);
    if (n > 0 && n < MAX_PATH)
        _snwprintf(path, count - 1, L"%s\\vlc\\xmeye-zoom.ini", appdata);
    else
        _snwprintf(path, count - 1, L"xmeye-zoom.ini");
    path[count - 1] = 0;
}

static int load_setting(const wchar_t *section, const wchar_t *key, int fallback)
{
    wchar_t path[MAX_PATH];
    settings_path(path, MAX_PATH);
    return (int)GetPrivateProfileIntW(section, key, fallback, path);
}

static void save_setting(const wchar_t *section, const wchar_t *key, int value)
{
    wchar_t path[MAX_PATH], text[32];
    settings_path(path, MAX_PATH);
    _snwprintf(text, 31, L"%d", value);
    text[31] = 0;
    WritePrivateProfileStringW(section, key, text, path);
}

static void status_text(intf_sys_t *sys, const wchar_t *text)
{
    if (sys && sys->status)
        SetWindowTextW(sys->status, text);
}

static vout_thread_t *current_vout(intf_thread_t *intf)
{
    playlist_t *playlist = pl_Get(intf);
    input_thread_t *input = playlist_CurrentInput(playlist);
    if (!input)
        return NULL;
    vout_thread_t *vout = input_GetVout(input);
    vlc_object_release(input);
    return vout;
}

static void center_limits(const intf_sys_t *sys, int *minc, int *maxc)
{
    int half = (5000 + sys->zoom / 2) / sys->zoom;
    half = clamp_int(half, 0, 50);
    *minc = half;
    *maxc = 100 - half;
}

static void clamp_center(intf_sys_t *sys)
{
    int lo, hi;
    center_limits(sys, &lo, &hi);
    sys->cx = clamp_int(sys->cx, lo, hi);
    sys->cy = clamp_int(sys->cy, lo, hi);
}

static void sync_controls(intf_sys_t *sys)
{
    if (!sys || !sys->window)
        return;
    sys->applying = true;
    SendMessageW(sys->zoom_track, TBM_SETPOS, TRUE, sys->zoom);
    set_edit_int(sys->zoom_edit, sys->zoom);
    set_edit_int(sys->center_x, sys->cx);
    set_edit_int(sys->center_y, sys->cy);
    set_edit_int(sys->source_w, sys->source_width);
    set_edit_int(sys->source_h, sys->source_height);
    sys->applying = false;
    InvalidateRect(sys->window, NULL, FALSE);
}

static void show_zoom_osd(vout_thread_t *vout, int zoom)
{
    char text[64];
    snprintf(text, sizeof(text), "Zoom %d%%", zoom);
    vout_OSDText(vout, VOUT_SPU_CHANNEL_OSD,
                 SUBPICTURE_ALIGN_BOTTOM | SUBPICTURE_ALIGN_LEFT,
                 VLC_TICK_FROM_MS(900), text);
}

static void apply_zoom(intf_thread_t *intf, bool show_osd)
{
    intf_sys_t *sys = intf->p_sys;
    if (!sys || !sys->window)
        return;

    sys->zoom = clamp_int(sys->zoom, ZOOM_MIN, ZOOM_MAX);
    sys->source_width = clamp_int(sys->source_width, 16, 10000);
    sys->source_height = clamp_int(sys->source_height, 16, 10000);
    clamp_center(sys);
    sync_controls(sys);

    vout_thread_t *vout = current_vout(intf);
    if (!vout)
    {
        status_text(sys, L"No hay vídeo activo. Los controles quedan preparados.");
        return;
    }

    bool enabled = SendMessageW(GetDlgItem(sys->window, ID_ZOOM_ACTIVE),
                                BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (!enabled || sys->zoom <= 100)
    {
        var_SetString(vout, "crop", "0+0+0+0");
        var_SetBool(vout, "autoscale", true);
        if (show_osd)
            show_zoom_osd(vout, 100);
        vlc_object_release(vout);
        status_text(sys, enabled ? L"Zoom 100% · imagen completa." : L"Zoom desactivado.");
        return;
    }

    int vw = (sys->source_width * 100 + sys->zoom / 2) / sys->zoom;
    int vh = (sys->source_height * 100 + sys->zoom / 2) / sys->zoom;
    vw = clamp_int(vw, 2, sys->source_width);
    vh = clamp_int(vh, 2, sys->source_height);

    int left = (sys->cx * sys->source_width) / 100 - vw / 2;
    int top = (sys->cy * sys->source_height) / 100 - vh / 2;
    left = clamp_int(left, 0, sys->source_width - vw);
    top = clamp_int(top, 0, sys->source_height - vh);
    int right = sys->source_width - vw - left;
    int bottom = sys->source_height - vh - top;

    char crop[128];
    snprintf(crop, sizeof(crop), "%d+%d+%d+%d", left, top, right, bottom);
    var_SetString(vout, "crop", crop);
    var_SetBool(vout, "autoscale", true);
    if (show_osd)
        show_zoom_osd(vout, sys->zoom);
    vlc_object_release(vout);

    wchar_t status[192];
    _snwprintf(status, 191,
               L"Zoom %d%% · centro %d/%d · ROI %dx%d · fuente %dx%d",
               sys->zoom, sys->cx, sys->cy, vw, vh,
               sys->source_width, sys->source_height);
    status[191] = 0;
    status_text(sys, status);
}

static bool detect_source_size(intf_thread_t *intf, bool announce)
{
    intf_sys_t *sys = intf->p_sys;
    vout_thread_t *vout = current_vout(intf);
    if (!vout)
    {
        if (announce) status_text(sys, L"No hay vídeo activo para detectar W/H.");
        return false;
    }

    picture_t *picture = NULL;
    int ret = vout_GetSnapshot(vout, NULL, &picture, NULL, NULL,
                               VLC_TICK_FROM_MS(350));
    vlc_object_release(vout);
    if (ret != VLC_SUCCESS || !picture)
    {
        if (announce) status_text(sys, L"No se pudo detectar W/H; usa el override manual.");
        return false;
    }

    unsigned w = picture->format.i_visible_width;
    unsigned h = picture->format.i_visible_height;
    if (!w) w = picture->format.i_width;
    if (!h) h = picture->format.i_height;
    picture_Release(picture);

    if (w < 16 || h < 16 || w > 10000 || h > 10000)
    {
        if (announce) status_text(sys, L"Dimensiones detectadas fuera de rango; usa W/H manual.");
        return false;
    }

    sys->source_width = (int)w;
    sys->source_height = (int)h;
    sys->detected_once = true;
    sync_controls(sys);

    if (announce)
    {
        wchar_t text[128];
        _snwprintf(text, 127, L"Fuente detectada automáticamente: %ux%u.", w, h);
        text[127] = 0;
        status_text(sys, text);
    }
    return true;
}

static void save_state(intf_sys_t *sys)
{
    save_setting(L"zoom", L"enabled",
        SendMessageW(GetDlgItem(sys->window, ID_ZOOM_ACTIVE), BM_GETCHECK, 0, 0) == BST_CHECKED);
    save_setting(L"zoom", L"zoom", sys->zoom);
    save_setting(L"zoom", L"cx", sys->cx);
    save_setting(L"zoom", L"cy", sys->cy);
    save_setting(L"zoom", L"width", sys->source_width);
    save_setting(L"zoom", L"height", sys->source_height);
    save_setting(L"window", L"topmost",
        SendMessageW(GetDlgItem(sys->window, ID_TOPMOST), BM_GETCHECK, 0, 0) == BST_CHECKED);

    RECT wr;
    if (GetWindowRect(sys->window, &wr))
    {
        save_setting(L"window", L"x", wr.left);
        save_setting(L"window", L"y", wr.top);
    }
}

static void load_state(intf_sys_t *sys)
{
    sys->zoom = clamp_int(load_setting(L"zoom", L"zoom", 200), ZOOM_MIN, ZOOM_MAX);
    sys->cx = clamp_int(load_setting(L"zoom", L"cx", 50), 0, 100);
    sys->cy = clamp_int(load_setting(L"zoom", L"cy", 50), 0, 100);
    sys->source_width = clamp_int(load_setting(L"zoom", L"width", 2592), 16, 10000);
    sys->source_height = clamp_int(load_setting(L"zoom", L"height", 1944), 16, 10000);
    clamp_center(sys);
}

static void restore_saved_state(intf_thread_t *intf)
{
    intf_sys_t *sys = intf->p_sys;
    load_state(sys);
    bool enabled = load_setting(L"zoom", L"enabled", 1) != 0;
    SendMessageW(GetDlgItem(sys->window, ID_ZOOM_ACTIVE), BM_SETCHECK,
                 enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    sync_controls(sys);
    apply_zoom(intf, true);
}

static void reset_state(intf_thread_t *intf)
{
    intf_sys_t *sys = intf->p_sys;
    sys->zoom = 100;
    sys->cx = 50;
    sys->cy = 50;
    SendMessageW(GetDlgItem(sys->window, ID_ZOOM_ACTIVE), BM_SETCHECK, BST_CHECKED, 0);
    sync_controls(sys);
    apply_zoom(intf, true);
}

static void set_center_from_preview(intf_thread_t *intf, int x, int y)
{
    intf_sys_t *sys = intf->p_sys;
    RECT r; preview_rect(&r);
    int pw = r.right - r.left;
    int ph = r.bottom - r.top;
    if (pw <= 0 || ph <= 0)
        return;

    int rx = clamp_int(x - r.left, 0, pw);
    int ry = clamp_int(y - r.top, 0, ph);
    sys->cx = (rx * 100 + pw / 2) / pw;
    sys->cy = (ry * 100 + ph / 2) / ph;
    clamp_center(sys);
    sync_controls(sys);
    apply_zoom(intf, false);
}

static void change_zoom(intf_thread_t *intf, int zoom, bool osd)
{
    intf_sys_t *sys = intf->p_sys;
    sys->zoom = clamp_int(zoom, ZOOM_MIN, ZOOM_MAX);
    clamp_center(sys);
    sync_controls(sys);
    apply_zoom(intf, osd);
}

static void paint_preview(intf_sys_t *sys, HDC dc)
{
    RECT r; preview_rect(&r);

    HBRUSH bg = CreateSolidBrush(RGB(54, 54, 54));
    FillRect(dc, &r, bg);
    DeleteObject(bg);

    HPEN grid = CreatePen(PS_SOLID, 1, RGB(104, 104, 104));
    HPEN oldpen = SelectObject(dc, grid);
    for (int i = 1; i < 4; ++i)
    {
        int x = r.left + (r.right-r.left) * i / 4;
        int y = r.top + (r.bottom-r.top) * i / 4;
        MoveToEx(dc, x, r.top, NULL); LineTo(dc, x, r.bottom);
        MoveToEx(dc, r.left, y, NULL); LineTo(dc, r.right, y);
    }
    SelectObject(dc, oldpen);
    DeleteObject(grid);

    int zoom = clamp_int(sys->zoom, ZOOM_MIN, ZOOM_MAX);
    int rw = (r.right-r.left) * 100 / zoom;
    int rh = (r.bottom-r.top) * 100 / zoom;
    int mx = r.left + (r.right-r.left) * sys->cx / 100;
    int my = r.top + (r.bottom-r.top) * sys->cy / 100;
    RECT roi = { mx-rw/2, my-rh/2, mx+rw/2, my+rh/2 };

    if (roi.left < r.left) OffsetRect(&roi, r.left-roi.left, 0);
    if (roi.right > r.right) OffsetRect(&roi, r.right-roi.right, 0);
    if (roi.top < r.top) OffsetRect(&roi, 0, r.top-roi.top);
    if (roi.bottom > r.bottom) OffsetRect(&roi, 0, r.bottom-roi.bottom);

    HPEN white = CreatePen(PS_SOLID, 3, RGB(248,248,248));
    HBRUSH hollow = GetStockObject(HOLLOW_BRUSH);
    oldpen = SelectObject(dc, white);
    HBRUSH oldbrush = SelectObject(dc, hollow);
    Rectangle(dc, roi.left, roi.top, roi.right, roi.bottom);
    int cx = (roi.left + roi.right) / 2;
    int cy = (roi.top + roi.bottom) / 2;
    MoveToEx(dc, cx-10, cy, NULL); LineTo(dc, cx+10, cy);
    MoveToEx(dc, cx, cy-10, NULL); LineTo(dc, cx, cy+10);
    SelectObject(dc, oldbrush);
    SelectObject(dc, oldpen);
    DeleteObject(white);

    wchar_t text[128];
    _snwprintf(text, 127, L"%d%%   X %d%%   Y %d%%   · arrastra para mover · rueda para zoom",
               sys->zoom, sys->cx, sys->cy);
    text[127] = 0;
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(235,235,235));
    RECT tr = {r.left, r.bottom - 28, r.right, r.bottom - 6};
    DrawTextW(dc, text, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    intf_thread_t *intf = (intf_thread_t *)(uintptr_t)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    intf_sys_t *sys = intf ? intf->p_sys : NULL;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lp;
        intf = cs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)(uintptr_t)intf);
        sys = intf->p_sys;
    }

    switch (msg)
    {
        case WM_CREATE:
        {
            sys->window = hwnd;
            load_state(sys);

            HWND title = make_static(hwnd, 24, 14, 390, 30,
                                     L"VLC XMEye · Zoom interactivo", 0);
            sys->title_font = CreateFontW(-22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                          DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            SendMessageW(title, WM_SETFONT, (WPARAM)sys->title_font, TRUE);

            make_button(hwnd, 570, 18, 170, 24, L"Siempre encima", ID_TOPMOST, BS_AUTOCHECKBOX);

            make_static(hwnd, 24, 402, 72, 22, L"Zoom %", 0);
            sys->zoom_track = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ | TBS_AUTOTICKS,
                    92, 398, 430, 32, hwnd, (HMENU)(INT_PTR)ID_ZOOM_TRACK,
                    GetModuleHandleW(NULL), NULL);
            SendMessageW(sys->zoom_track, TBM_SETRANGE, TRUE, MAKELONG(ZOOM_MIN, ZOOM_MAX));
            SendMessageW(sys->zoom_track, TBM_SETTICFREQ, 50, 0);
            sys->zoom_edit = make_edit(hwnd, 532, 398, 62, 25, L"200", ID_ZOOM_EDIT);
            make_static(hwnd, 598, 402, 32, 22, L"%", 0);
            make_button(hwnd, 642, 397, 98, 27, L"100%", ID_RESET, BS_PUSHBUTTON);

            make_button(hwnd, 24, 438, 112, 26, L"Zoom activo", ID_ZOOM_ACTIVE, BS_AUTOCHECKBOX);
            SendMessageW(GetDlgItem(hwnd, ID_ZOOM_ACTIVE), BM_SETCHECK,
                         load_setting(L"zoom", L"enabled", 1) ? BST_CHECKED : BST_UNCHECKED, 0);

            make_static(hwnd, 154, 442, 70, 22, L"Centro X", 0);
            sys->center_x = make_edit(hwnd, 224, 438, 54, 25, L"50", ID_CENTER_X);
            make_static(hwnd, 292, 442, 70, 22, L"Centro Y", 0);
            sys->center_y = make_edit(hwnd, 362, 438, 54, 25, L"50", ID_CENTER_Y);
            make_button(hwnd, 440, 437, 96, 27, L"Centrar", ID_CENTER, BS_PUSHBUTTON);
            make_button(hwnd, 548, 437, 92, 27, L"Guardar", ID_SAVE, BS_PUSHBUTTON);
            make_button(hwnd, 648, 437, 92, 27, L"Recuperar", ID_RESTORE, BS_PUSHBUTTON);

            make_static(hwnd, 24, 482, 72, 22, L"Fuente W", 0);
            sys->source_w = make_edit(hwnd, 96, 478, 74, 25, L"2592", ID_SOURCE_W);
            make_static(hwnd, 190, 482, 72, 22, L"Fuente H", 0);
            sys->source_h = make_edit(hwnd, 262, 478, 74, 25, L"1944", ID_SOURCE_H);
            make_button(hwnd, 356, 477, 170, 27, L"Detectar W/H", ID_SOURCE_AUTO, BS_PUSHBUTTON);
            make_static(hwnd, 548, 481, 192, 22, L"manual = override", 0);

            sys->status = make_static(hwnd, 24, 522, 716, 24,
                                      L"Listo. Arrastra el ROI o usa la rueda sobre el mapa.", ID_STATUS);
            make_static(hwnd, 24, 554, 430, 18,
                        L"MVP: crop + autoscale · sin recodificar · VLC 3.0.24 x64", 0);
            sys->credit = CreateWindowExW(0, L"STATIC",
                        L"Álvaro Ibarz · github.com/aibarz/vlc-xmeye-interactive-zoom",
                        WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_RIGHT,
                        430, 552, 310, 20, hwnd, (HMENU)(INT_PTR)ID_CREDIT,
                        GetModuleHandleW(NULL), NULL);
            sys->credit_font = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE,
                                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                           CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                           DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            SendMessageW(sys->credit, WM_SETFONT, (WPARAM)sys->credit_font, TRUE);

            bool topmost = load_setting(L"window", L"topmost", 0) != 0;
            SendMessageW(GetDlgItem(hwnd, ID_TOPMOST), BM_SETCHECK,
                         topmost ? BST_CHECKED : BST_UNCHECKED, 0);
            if (topmost)
                SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);

            sync_controls(sys);
            SetTimer(hwnd, 1, 900, NULL);
            SetEvent(sys->ready);
            return 0;
        }

        case WM_TIMER:
            if (wp == 1 && !sys->detected_once)
            {
                if (detect_source_size(intf, false))
                {
                    KillTimer(hwnd, 1);
                    apply_zoom(intf, false);
                }
            }
            return 0;

        case WM_HSCROLL:
            if ((HWND)lp == sys->zoom_track)
            {
                change_zoom(intf, (int)SendMessageW(sys->zoom_track, TBM_GETPOS, 0, 0), true);
                return 0;
            }
            break;

        case WM_LBUTTONDOWN:
        {
            RECT r; preview_rect(&r);
            POINT p = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            if (PtInRect(&r, p))
            {
                sys->dragging = true;
                SetCapture(hwnd);
                set_center_from_preview(intf, p.x, p.y);
                return 0;
            }
            break;
        }

        case WM_MOUSEMOVE:
            if (sys->dragging && (wp & MK_LBUTTON))
            {
                set_center_from_preview(intf, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                return 0;
            }
            break;

        case WM_LBUTTONUP:
            if (sys->dragging)
            {
                sys->dragging = false;
                ReleaseCapture();
                set_center_from_preview(intf, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                return 0;
            }
            break;

        case WM_MOUSEWHEEL:
        {
            POINT p = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            ScreenToClient(hwnd, &p);
            RECT r; preview_rect(&r);
            if (PtInRect(&r, p))
            {
                int delta = GET_WHEEL_DELTA_WPARAM(wp);
                int steps = delta / WHEEL_DELTA;
                if (!steps) steps = delta > 0 ? 1 : -1;
                change_zoom(intf, sys->zoom + steps * ZOOM_STEP, true);
                return 0;
            }
            break;
        }

        case WM_COMMAND:
        {
            int id = LOWORD(wp);
            int event = HIWORD(wp);

            if (id == ID_TOPMOST && event == BN_CLICKED)
            {
                bool on = SendMessageW(GetDlgItem(hwnd, ID_TOPMOST), BM_GETCHECK, 0, 0) == BST_CHECKED;
                SetWindowPos(hwnd, on ? HWND_TOPMOST : HWND_NOTOPMOST,
                             0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
                return 0;
            }
            if (id == ID_ZOOM_ACTIVE && event == BN_CLICKED)
            {
                apply_zoom(intf, true);
                return 0;
            }
            if (id == ID_CENTER && event == BN_CLICKED)
            {
                sys->cx = 50; sys->cy = 50; clamp_center(sys);
                sync_controls(sys); apply_zoom(intf, false);
                return 0;
            }
            if (id == ID_RESET && event == BN_CLICKED)
            {
                reset_state(intf);
                return 0;
            }
            if (id == ID_SOURCE_AUTO && event == BN_CLICKED)
            {
                if (detect_source_size(intf, true))
                    apply_zoom(intf, false);
                return 0;
            }
            if (id == ID_SAVE && event == BN_CLICKED)
            {
                save_state(sys);
                status_text(sys, L"Zoom, centro, W/H y ventana guardados.");
                return 0;
            }
            if (id == ID_RESTORE && event == BN_CLICKED)
            {
                restore_saved_state(intf);
                status_text(sys, L"Estado guardado recuperado.");
                return 0;
            }
            if (id == ID_CREDIT && event == STN_CLICKED)
            {
                ShellExecuteW(hwnd, L"open",
                    L"https://github.com/aibarz/vlc-xmeye-interactive-zoom",
                    NULL, NULL, SW_SHOWNORMAL);
                return 0;
            }

            if ((id == ID_ZOOM_EDIT || id == ID_CENTER_X || id == ID_CENTER_Y ||
                 id == ID_SOURCE_W || id == ID_SOURCE_H) && event == EN_KILLFOCUS && !sys->applying)
            {
                sys->zoom = get_edit_int(sys->zoom_edit, sys->zoom, ZOOM_MIN, ZOOM_MAX);
                sys->cx = get_edit_int(sys->center_x, sys->cx, 0, 100);
                sys->cy = get_edit_int(sys->center_y, sys->cy, 0, 100);
                sys->source_width = get_edit_int(sys->source_w, sys->source_width, 16, 10000);
                sys->source_height = get_edit_int(sys->source_h, sys->source_height, 16, 10000);
                clamp_center(sys);
                sync_controls(sys);
                apply_zoom(intf, id == ID_ZOOM_EDIT);
                return 0;
            }
            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            paint_preview(sys, dc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_CLOSE:
            save_state(sys);
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            sys->window = NULL;
            if (sys->title_font) DeleteObject(sys->title_font);
            if (sys->credit_font) DeleteObject(sys->credit_font);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void *UiThread(void *opaque)
{
    intf_thread_t *intf = opaque;
    intf_sys_t *sys = intf->p_sys;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    HINSTANCE instance = GetModuleHandleW(NULL);
    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"VLCXMEyeInteractiveZoom";
    RegisterClassExW(&wc);

    int wx = load_setting(L"window", L"x", CW_USEDEFAULT);
    int wy = load_setting(L"window", L"y", CW_USEDEFAULT);
    if (wx != CW_USEDEFAULT && wy != CW_USEDEFAULT)
    {
        int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
        int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        if (wx < vx - 700 || wx > vx + vw - 80 || wy < vy - 500 || wy > vy + vh - 40)
        {
            wx = CW_USEDEFAULT;
            wy = CW_USEDEFAULT;
        }
    }

    HWND window = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"VLC XMEye · Interactive Zoom",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        wx, wy, 790, 625,
        NULL, NULL, instance, intf);

    if (!window)
    {
        SetEvent(sys->ready);
        return NULL;
    }

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    MSG m;
    while (GetMessageW(&m, NULL, 0, 0) > 0)
    {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return NULL;
}

static int Open(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj;
    intf_sys_t *sys = calloc(1, sizeof(*sys));
    if (!sys)
        return VLC_ENOMEM;

    intf->p_sys = sys;
    sys->ready = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!sys->ready)
    {
        free(sys);
        return VLC_EGENERIC;
    }

    if (vlc_clone(&sys->thread, UiThread, intf, VLC_THREAD_PRIORITY_LOW))
    {
        CloseHandle(sys->ready);
        free(sys);
        return VLC_ENOMEM;
    }

    WaitForSingleObject(sys->ready, INFINITE);
    CloseHandle(sys->ready);
    sys->ready = NULL;

    if (!sys->window)
    {
        vlc_join(sys->thread, NULL);
        free(sys);
        return VLC_EGENERIC;
    }
    return VLC_SUCCESS;
}

static void Close(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj;
    intf_sys_t *sys = intf->p_sys;
    if (!sys)
        return;

    if (sys->window)
        PostMessageW(sys->window, WM_CLOSE, 0, 0);
    vlc_join(sys->thread, NULL);
    free(sys);
}

vlc_module_begin()
    set_shortname(N_("XMEye Zoom"))
    set_description(N_("Interactive configurable zoom for VLC"))
    set_category(CAT_INTERFACE)
    set_subcategory(SUBCAT_INTERFACE_CONTROL)
    set_capability("interface", 0)
    set_callbacks(Open, Close)
    add_shortcut("xmeyezoom")
vlc_module_end()
