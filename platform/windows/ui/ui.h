#ifndef TELEGRAMCHATEXPORTER_UI_H
#define TELEGRAMCHATEXPORTER_UI_H
#include <string>
#include <functional>

namespace exporter {
    struct UI {
        float x = 0;
        float y = 0;
        std::wstring text;
    };

    struct Label : public UI {};

    struct Button : public UI {
        std::function<void()> on_click;
    };
}


#endif //TELEGRAMCHATEXPORTER_UI_H
