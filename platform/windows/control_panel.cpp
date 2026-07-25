#include "control_panel.h"

#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

static LRESULT CALLBACK panelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            // TODO: this is where the current Scene's UI elements get drawn.
            EndPaint(hwnd, &ps);
            return 0;
        }
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}


namespace exporter {
    void ControlPanel::run() {
        const wchar_t * className = L"TelegramChatExporterControlPanel";

        WNDCLASSW wc{};
        wc.lpfnWndProc = panelProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = className;
        wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
        RegisterClassW(&wc);

        constexpr DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        hwnd_ = CreateWindowExW(0, className, L"TelegramChatExporter", style, CW_USEDEFAULT, CW_USEDEFAULT, winWidth, winHeight, nullptr, nullptr, wc.hInstance, nullptr);

        constexpr BOOL useDarkMode = TRUE;
        DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
        SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        ShowWindow(hwnd_, SW_SHOW);
    }

    void ControlPanel::close() const {
        ShowWindow(hwnd_, SW_HIDE);
    }

}  // namespace exporter
