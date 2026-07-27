#ifndef TELEGRAMCHATEXPORTER_SCENE_H
#define TELEGRAMCHATEXPORTER_SCENE_H
#include <list>
#include <memory>
#include "../ui/ui.h"

namespace exporter {
    constexpr unsigned short uiLabelNum = 4;
    constexpr unsigned short uiButtonNum = 4;

    struct Scene {
        std::list<exporter::Label> uiLabels = {};
        std::list<exporter::Button> uiButtons = {};
        std::list<exporter::Button>::iterator uiButtonsIterator;

        virtual ~Scene() = default;
    };
}




#endif //TELEGRAMCHATEXPORTER_SCENE_H
