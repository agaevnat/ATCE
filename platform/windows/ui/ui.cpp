#include "ui.h"

namespace exporter {
    void UI::setFontSize(const unsigned short size) {
        this->fontSize = size;
        this->hFont = CreateFontW(
            this->fontSize,
            0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Consolas"
        );
        this->hFontUnderline = CreateFontW(
            this->fontSize,
            0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
            FALSE, TRUE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Consolas"
        );
    }


    void UI::setColor(const COLORREF color) {
        this->fontColor = color;
    }


    void UI::draw(HDC hdc) const {
        SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, this->fontColor);
        TextOutW(hdc, static_cast<int>(this->x), static_cast<int>(this->y), this->text.c_str(), static_cast<int>(this->text.length()));
    }

    void Button::draw(HDC hdc) const {
        SelectObject(hdc, this->selected ? hFontUnderline : hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, this->selected ? kColorCyan : this->fontColor);
        TextOutW(hdc, static_cast<int>(this->x), static_cast<int>(this->y), this->text.c_str(), static_cast<int>(this->text.length()));
    }
}
