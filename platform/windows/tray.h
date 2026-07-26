#pragma once

#include <functional>
#include <string>
#include <vector>
#include <array>
#include <windows.h>

namespace exporter {
    constexpr unsigned short trayMenuOptionNum = 2;
    inline bool tray_menu_initialized = false;

    struct TrayMenuItem {
        std::wstring label;
        std::function<void()> on_click;
    };

    class TrayMenu {
    private:
        std::array<TrayMenuItem, trayMenuOptionNum> items;
        HWND hwnd_ = nullptr;

    public:
        void terminate() const;
        int run(const std::wstring &, const std::array<TrayMenuItem, trayMenuOptionNum> &);
    };

}  // namespace exporter
