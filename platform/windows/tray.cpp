#include "platform/windows/tray.h"
#include <windows.h>
#include <shellapi.h>
#include <cwchar>

static constexpr UINT kTrayCallbackMessage = WM_APP + 1;
static constexpr UINT kTrayIconId = 1;
static constexpr UINT kFirstMenuCommandId = 1000;
static NOTIFYICONDATAW gIconData {};

static const std::array<exporter::TrayMenuItem, exporter::trayMenuOptionNum> * trayMenuItems = nullptr;


static void show_context_menu(HWND hwnd) {
    HMENU menu = CreatePopupMenu();

    for (size_t i = 0; i < trayMenuItems->size(); ++i) {
        AppendMenuW(menu, MF_STRING, kFirstMenuCommandId + static_cast<UINT>(i), (*trayMenuItems)[i].label.c_str());
    }

    POINT cursor;
    GetCursorPos(&cursor);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hwnd, nullptr);
    PostMessage(hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
}


static LRESULT tray_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case kTrayCallbackMessage:
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
                show_context_menu(hwnd);
            }
            return 0;
        case WM_COMMAND: {
            UINT id = LOWORD(wParam);
            if (trayMenuItems != nullptr && id >= kFirstMenuCommandId && id < kFirstMenuCommandId + trayMenuItems->size()) {
                (*trayMenuItems)[id - kFirstMenuCommandId].on_click();
            }
            return 0;
        }
        case WM_DESTROY:
            Shell_NotifyIconW(NIM_DELETE, &gIconData);
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}


namespace exporter {
    void TrayMenu::terminate() const {
        DestroyWindow(hwnd_);
    }

    int TrayMenu::run(const std::wstring & tooltips, const std::array<TrayMenuItem, trayMenuOptionNum> & items) {
        trayMenuItems = &items;

        const wchar_t * trayName = L"TelegramChatExporterTrayWindow";
        WNDCLASSW wc {};
        wc.lpfnWndProc = tray_proc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = trayName;
        RegisterClassW(&wc);

        HWND hwnd = CreateWindowW(trayName, L"TelegramChatExporter", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, nullptr, nullptr, wc.hInstance, nullptr);
        hwnd_ = hwnd;
        gIconData = NOTIFYICONDATAW{};
        gIconData.cbSize = sizeof(gIconData);
        gIconData.hWnd = hwnd;
        gIconData.uID = kTrayIconId;
        gIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        gIconData.uCallbackMessage = kTrayCallbackMessage;
        gIconData.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
        wcsncpy_s(gIconData.szTip, tooltips.c_str(), _TRUNCATE);
        Shell_NotifyIconW(NIM_ADD, &gIconData);

        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        return static_cast<int>(msg.wParam);
    }
}
