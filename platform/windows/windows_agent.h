#pragma once

#include <functional>
#include <string>
#include <vector>

namespace exporter {

// Best-effort single-instance check, backed by a named mutex held for the
// process's lifetime — used so a second launch doesn't spawn a second tray
// icon on top of an already-running one.
bool is_agent_running();
void mark_agent_running();

struct TrayMenuItem {
    std::wstring label;
    std::function<void()> on_click;
};

// Win32 wide-string APIs (menu labels, message boxes) need UTF-16; our
// strings (config.json, prompts) are UTF-8 throughout.
std::wstring utf8_to_wide(const std::string& text);

void show_message_box(const std::wstring& title, const std::wstring& text);

// Ends the tray icon's message loop (call from a menu item's on_click).
void quit_tray_icon();

// Creates a hidden window + tray icon, shows `items` as its right-click
// context menu, and runs the Win32 message loop until quit_tray_icon() is
// called. Blocks the calling thread — the actual export/sync work runs on
// a separate thread started before this is called.
int run_tray_icon(const std::wstring& tooltip, std::vector<TrayMenuItem> items);

}  // namespace exporter
