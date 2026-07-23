#pragma once

#include <functional>
#include <string>

namespace exporter {

// 0x00BBGGRR, matching Win32's COLORREF convention directly.
using MenuColor = unsigned long;

constexpr MenuColor kMenuColorDefault = 0x00FFFFFF;
constexpr MenuColor kMenuColorTitle = 0x00FFFFFF;
constexpr MenuColor kMenuColorBorder = 0x00808080;
constexpr MenuColor kMenuColorOption = 0x00FFFFFF;
constexpr MenuColor kMenuColorGood = 0x0000C800;
constexpr MenuColor kMenuColorBad = 0x000000E0;

// Text color for the currently-selected editable field — no background
// highlight, just a distinct font color.
constexpr MenuColor kMenuColorSelectedFg = 0x00FFC800;

// Creates the window (hidden) on first call, then shows and foregrounds it.
// Safe to call repeatedly.
void show_menu_window();

// Hides the window without destroying it or the process. This is what both
// the "0" menu choice and the window's own close button do — unlike a
// console window, we own this one's WM_CLOSE handling completely, so
// closing it is always safe.
void hide_menu_window();

// Appends a line of colored text, auto-scrolling to it. If `highlighted` is
// true, the line's text is drawn in kMenuColorSelectedFg instead of `color`
// — used for the currently-selected editable field.
void menu_println(const std::wstring& text, MenuColor color = kMenuColorDefault, bool highlighted = false);

void menu_clear();

// Called with the pressed character whenever the menu window (or its
// content area) has keyboard focus. '\r' is Enter.
void set_menu_key_handler(std::function<void(wchar_t)> handler);

}  // namespace exporter
