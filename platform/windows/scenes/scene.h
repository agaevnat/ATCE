#ifndef TELEGRAMCHATEXPORTER_SCENE_H
#define TELEGRAMCHATEXPORTER_SCENE_H
#include <list>
#include <memory>
#include "../ui/ui.h"

namespace exporter {
    constexpr unsigned short uiLabelNum = 1;
    constexpr unsigned short uiButtonNum = 3;

    struct Scene {
        std::list<exporter::Label> uiLabels = {};
        std::list<exporter::Button> uiButtons = {};

        virtual ~Scene() = default;
    };
}




#endif //TELEGRAMCHATEXPORTER_SCENE_H
