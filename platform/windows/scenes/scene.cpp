#include "scene.h"
#include "../../client/client.h"

namespace exporter {
    void refreshStatusScene(Scene & scene) {
        if (!scene.isStatusScene) {
            return;
        }

        const AutoExportStats stats = g_client->getAutoExportStats();

        if (!g_client->isAutoExportEnabled()) {
            scene.uiProgressLabelIt->text = L"Paused — press Start in the main menu.";
        } else if (g_client->getConfig().exportPath.empty()) {
            scene.uiProgressLabelIt->text = L"Idle — set an export path in Settings first.";
        } else if (stats.lastChatId != 0) {
            scene.uiProgressLabelIt->text = L"Watching for changes. " + std::to_wstring(stats.messagesWritten) + L" message(s) written, last: \"" + g_client->getChatTitleWide(stats.lastChatId) + L"\"";
        } else {
            scene.uiProgressLabelIt->text = L"Watching for changes. Nothing written yet.";
        }

        auto labelIt = scene.uiQueueLabelsBegin;
        auto pendingIt = stats.pendingChatIds.begin();
        const auto pendingEnd = stats.pendingChatIds.end();

        for (int slot = 0; slot < scene.uiQueueLabelSlots; ++slot, ++labelIt) {
            if (pendingIt == pendingEnd) {
                labelIt->text = L"";
                continue;
            }

            const bool isLastSlot = (slot == scene.uiQueueLabelSlots - 1);
            const auto remaining = std::distance(pendingIt, pendingEnd);

            if (isLastSlot && remaining > 1) {
                labelIt->text = L"...and " + std::to_wstring(remaining) + L" more";
            } else {
                labelIt->text = g_client->getChatTitleWide(*pendingIt);
                ++pendingIt;
            }
        }
    }
}