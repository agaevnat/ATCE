#include "control_panel.h"

#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

static exporter::ControlPanel * controlPanel = nullptr;

static LRESULT CALLBACK panelProc(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        case WM_KEYDOWN: {
            exporter::Scene & scene = controlPanel->scenes.top();
            auto & buttons = scene.uiButtons;

            switch (wParam) {
                case VK_UP:
                case VK_DOWN: {
                    buttons[scene.focus].selected = false;
                    if (wParam == VK_DOWN) {
                        scene.focus = (scene.focus == 0) ? buttons.size() - 1 : scene.focus - 1;
                    } else {
                        scene.focus = (scene.focus + 1) % buttons.size();
                    }
                    buttons[scene.focus].selected = true;
                    InvalidateRect(hwnd, nullptr, TRUE);
                    return 0;
                }
                case VK_RETURN: {
                    if (buttons[scene.focus].on_click) {
                        buttons[scene.focus].on_click();
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

            for (exporter::Label & uiLabel : controlPanel->scenes.top().uiLabels) {
                uiLabel.draw(hdc);
            }

            for (exporter::Button & uiButton : controlPanel->scenes.top().uiButtons) {
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
        hwnd_ = CreateWindowExW(0, className, L"TelegramChatExporter", style, CW_USEDEFAULT, CW_USEDEFAULT, winWidth, winHeight, nullptr, nullptr, wc.hInstance, nullptr);

        constexpr BOOL useDarkMode = TRUE;
        DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
        SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        HDC hdc = GetDC(hwnd_);

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

        mainMenu.uiLabels[0] = std::move(title);
        mainMenu.uiLabels[1] = std::move(line1);
        mainMenu.uiLabels[2] = std::move(line2);
        mainMenu.uiLabels[3] = std::move(hint);

        exporter::Button close, settings, exp, status;
        close.text = L"Close";
        settings.text = L"Settings";
        exp.text = L"Export";
        status.text = L"Status";

        close.setFontSize(25);
        settings.setFontSize(25);
        exp.setFontSize(25);
        status.setFontSize(25);

        exp.x = 90;
        exp.y = 110;
        status.x = 90;
        status.y = 190;
        settings.x = 90;
        settings.y = 270;
        close.x = 90;
        close.y = 350;

        mainMenu.uiButtons[0] = std::move(close);
        mainMenu.uiButtons[1] = std::move(settings);
        mainMenu.uiButtons[2] = std::move(status);
        mainMenu.uiButtons[3] = std::move(exp);

        ReleaseDC(hwnd_, hdc);
        this->scenes.emplace(std::move(mainMenu));
        this->scenes.top().uiButtons[this->scenes.top().focus].selected = true;

        controlPanel = this;
        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
        SetFocus(hwnd_);
    }

    void ControlPanel::close() const {
        ShowWindow(hwnd_, SW_HIDE);
    }

}  // namespace exporter
