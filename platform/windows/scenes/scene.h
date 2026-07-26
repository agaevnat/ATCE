#ifndef TELEGRAMCHATEXPORTER_SCENE_H
#define TELEGRAMCHATEXPORTER_SCENE_H
#include <array>
#include <memory>
#include "../ui/ui.h"

namespace exporter {
    constexpr unsigned short uiLabelNum = 4;
    constexpr unsigned short uiButtonNum = 4;

    struct Scene {
        size_t focus = 0;
        std::array<exporter::Label, uiLabelNum> uiLabels = {};
        std::array<exporter::Button, uiButtonNum> uiButtons = {};

        virtual ~Scene() = default;
    };
}




#endif //TELEGRAMCHATEXPORTER_SCENE_H
