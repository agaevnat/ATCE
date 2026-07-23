#include "platform/windows/menu_window.h"

#include <windows.h>

#include <dwmapi.h>
#include <richedit.h>
#include <uxtheme.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace exporter {

namespace {

constexpr int kWindowWidth = 640;
constexpr int kWindowHeight = 420;
constexpr COLORREF kBackgroundColor = RGB(0, 0, 0);

HWND g_window = nullptr;
HWND g_richedit = nullptr;
WNDPROC g_original_richedit_proc = nullptr;
std::function<void(wchar_t)> g_key_handler;

void append_colored(const std::wstring& text, MenuColor color, bool highlighted) {
    int len = GetWindowTextLengthW(g_richedit);
    SendMessageW(g_richedit, EM_SETSEL, len, len);

    CHARFORMAT2W format{};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_COLOR | CFM_BACKCOLOR;
    format.crTextColor = static_cast<COLORREF>(highlighted ? kMenuColorSelectedFg : color);
    format.crBackColor = kBackgroundColor;
    SendMessageW(g_richedit, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));

    SendMessageW(g_richedit, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
    SendMessageW(g_richedit, EM_SCROLLCARET, 0, 0);
}

// RichEdit stays enabled (so text renders with real colors instead of a
// grayed-out "disabled control" look) and read-only, but we still want
// digit keypresses routed to our own handler regardless of whether the
// user clicked into it — so its own WM_CHAR is intercepted here and
// swallowed rather than passed through to the default edit behavior.
LRESULT CALLBACK richedit_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_CHAR) {
        if (g_key_handler) {
            g_key_handler(static_cast<wchar_t>(wparam));
        }
        return 0;
    }
    return CallWindowProcW(g_original_richedit_proc, hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_SIZE: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            if (g_richedit != nullptr) {
                MoveWindow(g_richedit, 0, 0, rc.right, rc.bottom, TRUE);
            }
            return 0;
        }
        case WM_CHAR:
            if (g_key_handler) {
                g_key_handler(static_cast<wchar_t>(wparam));
            }
            return 0;
        case WM_CLOSE:
            // The whole point of this window over a console: we own this
            // handler completely, so closing it is always just hiding it.
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

void ensure_window_created() {
    if (g_window != nullptr) {
        return;
    }

    LoadLibraryW(L"Msftedit.dll");

    const wchar_t* class_name = L"TelegramChatExporterMenuWindow";
    WNDCLASSW wc{};
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = class_name;
    wc.hbrBackground = CreateSolidBrush(kBackgroundColor);
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));  // IDC_ARROW
    RegisterClassW(&wc);

    g_window = CreateWindowExW(0, class_name, L"TelegramChatExporter", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                               CW_USEDEFAULT, kWindowWidth, kWindowHeight, nullptr, nullptr, wc.hInstance, nullptr);

    BOOL use_dark_mode = TRUE;
    DwmSetWindowAttribute(g_window, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark_mode, sizeof(use_dark_mode));
    // DwmSetWindowAttribute alone doesn't always repaint the title bar
    // immediately — force a non-client area refresh so it's dark from the
    // very first ShowWindow instead of only after some later frame change.
    SetWindowPos(g_window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    g_richedit = CreateWindowExW(0, MSFTEDIT_CLASS, L"",
                                 WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 0,
                                 0, kWindowWidth, kWindowHeight, g_window, nullptr, wc.hInstance, nullptr);

    // Standard scrollbars don't follow the window's own dark-mode setting —
    // this is the documented way (SetWindowTheme with this specific theme
    // name) to get a dark scrollbar on a plain Edit/RichEdit control.
    SetWindowTheme(g_richedit, L"DarkMode_Explorer", nullptr);

    SendMessageW(g_richedit, EM_SETBKGNDCOLOR, 0, kBackgroundColor);
    // Padding so text never sits flush against the window edge.
    SendMessageW(g_richedit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(14, 14));

    HFONT font = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH, L"Consolas");
    SendMessageW(g_richedit, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

    g_original_richedit_proc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(g_richedit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(richedit_subclass_proc)));
}

}  // namespace

void show_menu_window() {
    ensure_window_created();
    ShowWindow(g_window, SW_SHOW);
    SetForegroundWindow(g_window);
    SetFocus(g_window);
}

void hide_menu_window() {
    if (g_window != nullptr) {
        ShowWindow(g_window, SW_HIDE);
    }
}

void menu_println(const std::wstring& text, MenuColor color, bool highlighted) {
    ensure_window_created();
    append_colored(text + L"\r\n", color, highlighted);
}

void menu_clear() {
    ensure_window_created();
    SetWindowTextW(g_richedit, L"");
}

void set_menu_key_handler(std::function<void(wchar_t)> handler) {
    g_key_handler = std::move(handler);
}

}  // namespace exporter
