#include "control_panel.h"

#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

exporter::ControlPanel * exporter::g_controlPanel = nullptr;

static LRESULT CALLBACK panelProc(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        case WM_KEYDOWN: {
            exporter::Scene & scene = exporter::g_controlPanel->scenes.top();
            auto & buttons = scene.uiButtons;
            auto & it = scene.uiButtonsIterator;

            switch (wParam) {
                case VK_UP: {
                    if (it != buttons.begin()) {
                        it->selected = false;
                        it = std::prev(it);
                        it->selected = true;
                        InvalidateRect(hwnd, nullptr, TRUE);
                    }
                    return 0;
                }
                case VK_DOWN: {
                    if (std::next(it) != buttons.end()) {
                        it->selected = false;
                        it = std::next(it);
                        it->selected = true;
                        InvalidateRect(hwnd, nullptr, TRUE);
                    }
                    return 0;
                }
                case VK_RETURN: {
                    if (it->on_click) {
                        it->on_click();
                    }
                    return 0;
                }
                case VK_ESCAPE:
                    ShowWindow(hwnd, SW_HIDE);
                    return 0;
                default:
                    return 0;
            }
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            for (exporter::Label & uiLabel : exporter::g_controlPanel->scenes.top().uiLabels) {
                uiLabel.draw(hdc);
            }

            for (exporter::Button & uiButton : exporter::g_controlPanel->scenes.top().uiButtons) {
                uiButton.draw(hdc);
            }

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
        this->hwnd_ = CreateWindowExW(0, className, L"TelegramChatExporter", style, CW_USEDEFAULT, CW_USEDEFAULT, winWidth, winHeight, nullptr, nullptr, wc.hInstance, nullptr);

        constexpr BOOL useDarkMode = TRUE;
        DwmSetWindowAttribute(this->hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
        SetWindowPos(this->hwnd_, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        HDC hdc = GetDC(this->hwnd_);

        exporter::Scene mainMenu;
        exporter::Label title;
        title.text = L"Control Panel";
        SelectObject(hdc, title.hFont);
        SIZE size;
        GetTextExtentPoint32W(hdc, title.text.c_str(), static_cast<int>(title.text.length()), &size);
        title.x = static_cast<float>(exporter::winWidth - size.cx) / 2.0f;
        title.y = 30;

        exporter::Label line1, line2;
        line1.text = L"—————————————————————————————————————————————————————————————————";
        line2.text = L"—————————————————————————————————————————————————————————————————";
        line1.setFontSize(15);
        line2.setFontSize(15);
        SelectObject(hdc, line1.hFont);
        GetTextExtentPoint32W(hdc, line1.text.c_str(), static_cast<int>(line1.text.length()), &size);
        line1.x = static_cast<float>(exporter::winWidth - size.cx) / 2.0f;
        line1.y = 70;
        line2.x = static_cast<float>(exporter::winWidth - size.cx) / 2.0f;
        line2.y = 400;

        exporter::Label hint;
        hint.text = L"Use  ↑,  ↓  to navigate. Use ↵ to select. Use ESC to close.";
        hint.setFontSize(16);
        hint.setColor(exporter::kColorGreen);
        hint.x = 50;
        hint.y = 430;

        mainMenu.uiLabels.emplace_back(std::move(title));
        mainMenu.uiLabels.emplace_back(std::move(line1));
        mainMenu.uiLabels.emplace_back(std::move(line2));
        mainMenu.uiLabels.emplace_back(std::move(hint));

        exporter::Button close, settings, exp, status;
        close.text = L"Close";
        settings.text = L"Settings";
        exp.text = L"Export";
        status.text = L"Status";

        close.setFontSize(25);
        settings.setFontSize(25);
        exp.setFontSize(25);
        status.setFontSize(25);

        close.on_click = uiClosePanel;
        settings.on_click = uiOpenSettingsPage;
        exp.on_click = uiOpenExportPage;
        status.on_click = uiOpenStatusPage;

        exp.x = 90;
        exp.y = 110;
        status.x = 90;
        status.y = 190;
        settings.x = 90;
        settings.y = 270;
        close.x = 90;
        close.y = 350;

        mainMenu.uiButtons.emplace_back(std::move(exp));
        mainMenu.uiButtons.emplace_back(std::move(status));
        mainMenu.uiButtons.emplace_back(std::move(settings));
        mainMenu.uiButtons.emplace_back(std::move(close));

        ReleaseDC(this->hwnd_, hdc);
        this->scenes.emplace(std::move(mainMenu));
        this->scenes.top().uiButtonsIterator = this->scenes.top().uiButtons.begin();
        this->scenes.top().uiButtonsIterator->selected = true;

        exporter::g_controlPanel = this;
        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
        SetFocus(hwnd_);
    }

    void ControlPanel::close() const {
        ShowWindow(hwnd_, SW_HIDE);
    }

}  // namespace exporter
