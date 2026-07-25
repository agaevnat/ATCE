#ifndef TELEGRAMCHATEXPORTER_SCENE_H
#define TELEGRAMCHATEXPORTER_SCENE_H
#include <array>
#include "../ui/ui.h"

namespace exporter {
    constexpr unsigned short uiElemNum = 2;

    class Scene {
        std::array<UI, uiElemNum> uiElements;
    };

    class MainMenu : public Scene {

    };

    class Settings : public Scene {

    };
}




#endif //TELEGRAMCHATEXPORTER_SCENE_H
