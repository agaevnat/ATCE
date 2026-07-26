#ifndef TELEGRAMCHATEXPORTER_CONTROL_PANEL_H
#define TELEGRAMCHATEXPORTER_CONTROL_PANEL_H
#include <stack>
#include <memory>

#include "scenes/scene.h"


namespace exporter {
    constexpr size_t winWidth = 600;
    constexpr size_t winHeight = 600;

    struct ControlPanel {
        std::stack<Scene> scenes;
        HWND hwnd_ = nullptr;

        void run();
        void close() const;
    };

    extern ControlPanel * g_controlPanel;
}

#endif //TELEGRAMCHATEXPORTER_CONTROL_PANEL_H
