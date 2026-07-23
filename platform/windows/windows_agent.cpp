#include "platform/windows/windows_agent.h"

#include <windows.h>

#include <shellapi.h>

#include <cwchar>

namespace exporter {

namespace {

// "Local\" scopes the mutex to the current user session, matching this
// being a per-user background agent rather than a system-wide service.
const wchar_t* kAgentMutexName = L"Local\\TelegramChatExporterAgentMutex";
HANDLE g_agent_mutex = nullptr;

constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT kTrayIconId = 1;
constexpr UINT kFirstMenuCommandId = 1000;

std::vector<TrayMenuItem>* g_items = nullptr;
NOTIFYICONDATAW g_icon_data{};

void show_context_menu(HWND hwnd) {
    HMENU menu = CreatePopupMenu();
    for (size_t i = 0; i < g_items->size(); ++i) {
        AppendMenuW(menu, MF_STRING, kFirstMenuCommandId + static_cast<UINT>(i), (*g_items)[i].label.c_str());
    }

    POINT cursor;
    GetCursorPos(&cursor);

    // Required so the popup menu closes properly when it loses focus.
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hwnd, nullptr);
    PostMessage(hwnd, WM_NULL, 0, 0);

    DestroyMenu(menu);
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case kTrayCallbackMessage:
            if (lparam == WM_RBUTTONUP || lparam == WM_LBUTTONUP) {
                show_context_menu(hwnd);
            }
            return 0;
        case WM_COMMAND: {
            UINT id = LOWORD(wparam);
            if (g_items != nullptr && id >= kFirstMenuCommandId && id < kFirstMenuCommandId + g_items->size()) {
                (*g_items)[id - kFirstMenuCommandId].on_click();
            }
            return 0;
        }
        case WM_DESTROY:
            Shell_NotifyIconW(NIM_DELETE, &g_icon_data);
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

}  // namespace

bool is_agent_running() {
    HANDLE existing = OpenMutexW(SYNCHRONIZE, FALSE, kAgentMutexName);
    if (existing == nullptr) {
        return false;
    }
    CloseHandle(existing);
    return true;
}

void mark_agent_running() {
    // Leaked deliberately: released by Windows when the process exits,
    // which is exactly when is_agent_running() should start reporting false.
    g_agent_mutex = CreateMutexW(nullptr, FALSE, kAgentMutexName);
}

std::wstring utf8_to_wide(const std::string& text) {
    if (text.empty()) {
        return std::wstring();
    }
    int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), size);
    return result;
}

void show_message_box(const std::wstring& title, const std::wstring& text) {
    MessageBoxW(nullptr, text.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}

void quit_tray_icon() {
    PostQuitMessage(0);
}

int run_tray_icon(const std::wstring& tooltip, std::vector<TrayMenuItem> items) {
    g_items = &items;

    const wchar_t* class_name = L"TelegramChatExporterTrayWindow";
    WNDCLASSW wc{};
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = class_name;
    RegisterClassW(&wc);

    // Never shown (no ShowWindow call) — exists only to own the tray icon
    // and receive its callback messages. A message-only (HWND_MESSAGE)
    // window doesn't work here: TrackPopupMenu/SetForegroundWindow need a
    // real top-level window to interact with focus correctly.
    HWND hwnd = CreateWindowW(class_name, L"TelegramChatExporter", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, nullptr, nullptr,
                              wc.hInstance, nullptr);

    g_icon_data = NOTIFYICONDATAW{};
    g_icon_data.cbSize = sizeof(g_icon_data);
    g_icon_data.hWnd = hwnd;
    g_icon_data.uID = kTrayIconId;
    g_icon_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_icon_data.uCallbackMessage = kTrayCallbackMessage;
    g_icon_data.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));  // IDI_APPLICATION
    wcsncpy_s(g_icon_data.szTip, tooltip.c_str(), _TRUNCATE);

    Shell_NotifyIconW(NIM_ADD, &g_icon_data);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

}  // namespace exporter
