#ifndef TELEGRAMCHATEXPORTER_CONTROL_PANEL_H
#define TELEGRAMCHATEXPORTER_CONTROL_PANEL_H
#include <stack>
#include <windows.h>


namespace exporter {
    constexpr size_t winWidth = 600;
    constexpr size_t winHeight = 600;

    class ControlPanel {
    private:
        HWND hwnd_ = nullptr;

    public:
        void run();
        void close() const;

    };
}

#endif //TELEGRAMCHATEXPORTER_CONTROL_PANEL_H
