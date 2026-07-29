#ifndef TELEGRAMCHATEXPORTER_SCENE_H
#define TELEGRAMCHATEXPORTER_SCENE_H
#include <list>
#include <memory>
#include "../ui/ui.h"

namespace exporter {
    struct Scene {
        std::list<exporter::Label> uiLabels = {};
        std::list<exporter::Button> uiButtons = {};
        std::list<std::unique_ptr<exporter::Shape>> uiShapes = {};
        std::list<exporter::Button>::iterator uiButtonsIterator;

        bool isStatusScene = false;
        std::list<exporter::Label>::iterator uiProgressLabelIt;
        std::list<exporter::Label>::iterator uiQueueLabelsBegin;
        int uiQueueLabelSlots = 0;
    };

    void refreshStatusScene(Scene & scene);
}




#endif //TELEGRAMCHATEXPORTER_SCENE_H
