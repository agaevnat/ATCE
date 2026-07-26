#include "ui.h"

namespace exporter {
    void UI::draw(HDC hdc) const {
        SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, kColorWhite);
        TextOutW(hdc, this->x, this->y, this->text.c_str(), this->text.length());
    }

    void Button::draw(HDC hdc) const {
        SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, this->selected ? kColorCyan : kColorWhite);
        TextOutW(hdc, this->x, this->y, this->text.c_str(), this->text.length());
    }
}
