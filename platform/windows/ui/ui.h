#ifndef TELEGRAMCHATEXPORTER_UI_H
#define TELEGRAMCHATEXPORTER_UI_H
#include <string>
#include <functional>
#include <windows.h>

namespace exporter {
    constexpr COLORREF kColorRed   = RGB(255, 0, 0);
    constexpr COLORREF kColorGreen = RGB(0, 180, 0);
    constexpr COLORREF kColorWhite = RGB(190, 190, 190);
    constexpr COLORREF kColorBlack = RGB(0, 0, 0);
    constexpr COLORREF kColorCyan  = RGB(0, 200, 200);

    struct UI {
        COLORREF fontColor = kColorWhite;
        float x = 0;
        float y = 0;
        bool bold = false;
        unsigned short fontSize = 25;
        std::wstring text;

        HFONT hFont = CreateFontW(
            fontSize,
            0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Consolas"
        );

        HFONT hFontUnderline = CreateFontW(
            fontSize,
            0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
            FALSE, TRUE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Consolas"
        );


        virtual ~UI() = default;
        virtual void draw(HDC hdc) const;
        void setFontSize(unsigned short size);
        void setColor(COLORREF color);
    };

    struct Label : public UI {};

    struct Button : public UI {
        bool selected = false;
        std::function<void()> on_click;

        void draw(HDC hdc) const override;
    };
}


#endif //TELEGRAMCHATEXPORTER_UI_H
