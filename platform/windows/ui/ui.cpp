#include "ui.h"
#include "../control_panel.h"
#include "../../client/client.h"
#include <shobjidl.h>

namespace exporter {
    void UI::setFontSize(const unsigned short size) {
        this->fontSize = size;
        this->hFont = CreateFontW(
            this->fontSize,
            0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Consolas"
        );
        this->hFontUnderline = CreateFontW(
            this->fontSize,
            0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
            FALSE, TRUE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Consolas"
        );
    }


    void UI::setColor(const COLORREF color) {
        this->fontColor = color;
    }


    void UI::draw(HDC hdc) const {
        SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, this->fontColor);
        TextOutW(hdc, static_cast<int>(this->x), static_cast<int>(this->y), this->text.c_str(), static_cast<int>(this->text.length()));
    }

    void Button::draw(HDC hdc) const {
        SelectObject(hdc, this->selected ? hFontUnderline : hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, this->selected ? kColorCyan : this->fontColor);
        TextOutW(hdc, static_cast<int>(this->x), static_cast<int>(this->y), this->text.c_str(), static_cast<int>(this->text.length()));
    }


    void Line::draw(HDC hdc) const {
        const HPEN pen = CreatePen(PS_SOLID, 1, this->color);
        const HGDIOBJ oldPen = SelectObject(hdc, pen);
        MoveToEx(hdc, static_cast<int>(x1), static_cast<int>(y1), nullptr);
        LineTo(hdc, static_cast<int>(x2), static_cast<int>(y2));
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }



    void uiOpenStatusPage() {
        exporter::Scene scene;
        scene.isStatusScene = true;

        constexpr float boxLeft = 40, boxTop = 130, boxRight = 540, boxBottom = 400;

        auto top = std::make_unique<Line>();
        top->x1 = boxLeft;
        top->y1 = boxTop;
        top->x2 = boxRight;
        top->y2 = boxTop;

        auto bottom = std::make_unique<Line>();
        bottom->x1 = boxLeft;
        bottom->y1 = boxBottom;
        bottom->x2 = boxRight;
        bottom->y2 = boxBottom;

        auto left = std::make_unique<Line>();
        left->x1 = boxLeft;
        left->y1 = boxTop;
        left->x2 = boxLeft;
        left->y2 = boxBottom;

        auto right = std::make_unique<Line>();
        right->x1 = boxRight;
        right->y1 = boxTop;
        right->x2 = boxRight;
        right->y2 = boxBottom;

        scene.uiShapes.emplace_back(std::move(top));
        scene.uiShapes.emplace_back(std::move(bottom));
        scene.uiShapes.emplace_back(std::move(left));
        scene.uiShapes.emplace_back(std::move(right));

        exporter::Label title;
        title.text = L"Autonomous export status";
        title.setColor(exporter::kColorGreen);
        title.setFontSize(25);
        title.x = 40;
        title.y = 40;

        exporter::Label queueTitle;
        queueTitle.text = L"Pending updates:";
        queueTitle.setFontSize(18);
        queueTitle.x = boxLeft;
        queueTitle.y = boxTop - 26;

        exporter::Label progress;
        progress.text = L"Watching for changes.";
        progress.setColor(exporter::kColorGreen);
        progress.setFontSize(20);
        progress.x = 40;
        progress.y = boxBottom + 20;

        exporter::Label hint;
        hint.text = L"Press 'Esc' to go back.";
        hint.setColor(exporter::kColorGreen);
        hint.setFontSize(18);
        hint.x = 40;
        hint.y = 560;

        scene.uiLabels.emplace_back(std::move(title));
        scene.uiLabels.emplace_back(std::move(queueTitle));

        constexpr int kQueueSlots = 10;
        constexpr float kRowHeight = 24;
        for (int i = 0; i < kQueueSlots; ++i) {
            exporter::Label queueLine;
            queueLine.setFontSize(18);
            queueLine.x = boxLeft + 20;
            queueLine.y = boxTop + 15 + static_cast<float>(i) * kRowHeight;
            scene.uiLabels.push_back(std::move(queueLine));
        }
        scene.uiQueueLabelsBegin = std::prev(scene.uiLabels.end(), kQueueSlots);
        scene.uiQueueLabelSlots = kQueueSlots;

        scene.uiLabels.push_back(std::move(progress));
        scene.uiProgressLabelIt = std::prev(scene.uiLabels.end());

        scene.uiLabels.emplace_back(std::move(hint));

        scene.uiButtonsIterator = scene.uiButtons.begin();

        exporter::g_controlPanel->scenes.emplace(std::move(scene));
        exporter::refreshStatusScene(exporter::g_controlPanel->scenes.top());
    }


    void uiOpenSettingsPage() {
        exporter::Scene scene;

        exporter::Label title;
        title.text = L"Settings";
        title.setColor(exporter::kColorGreen);
        title.setFontSize(25);
        title.x = 40;
        title.y = 40;

        exporter::Label hint;
        hint.text = L"Press 'Esc' to go back.";
        hint.setColor(exporter::kColorGreen);
        hint.setFontSize(18);
        hint.x = 40;
        hint.y = 535;

        exporter::Label pathLabel;
        pathLabel.text = L"Export path: " + exporter::g_client->getConfig().exportPath;
        pathLabel.setFontSize(18);
        pathLabel.x = 40;
        pathLabel.y = 120;

        scene.uiLabels.emplace_back(std::move(title));
        scene.uiLabels.emplace_back(std::move(hint));
        scene.uiLabels.emplace_back(std::move(pathLabel));
        const auto pathLabelIt = std::prev(scene.uiLabels.end());

        exporter::Button browse;
        browse.text = L"Browse";
        browse.setFontSize(18);
        browse.x = 40;
        browse.y = 160;
        browse.on_click = [pathLabelIt]() {
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

            IFileDialog * fileDialog = nullptr;
            if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fileDialog)))) {
                DWORD options = 0;
                fileDialog->GetOptions(&options);
                fileDialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

                if (SUCCEEDED(fileDialog->Show(exporter::g_controlPanel->hwnd_))) {
                    IShellItem * item = nullptr;
                    if (SUCCEEDED(fileDialog->GetResult(&item))) {
                        PWSTR path = nullptr;
                        if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                            exporter::g_client->getConfig().exportPath = path;
                            pathLabelIt->text = L"Export path: " + exporter::g_client->getConfig().exportPath;
                            exporter::saveConfig(exporter::g_client->getConfig(), exporter::appFilePath("config.json"));
                            CoTaskMemFree(path);
                        }
                        item->Release();
                    }
                }
                fileDialog->Release();
            }

            CoUninitialize();
            InvalidateRect(exporter::g_controlPanel->hwnd_, nullptr, TRUE);
        };

        scene.uiButtons.push_back(std::move(browse));

        exporter::Config & config = exporter::g_client->getConfig();
        const std::pair<const wchar_t *, bool *> toggles[] = {
            {L"Personal Chats",  &config.personalChats},
            {L"Group Chats",     &config.groupChats},
            {L"Photos",          &config.photos},
            {L"Videos",          &config.videos},
            {L"Voices",          &config.voices},
            {L"Circle messages", &config.circleMessages},
            {L"Files",           &config.files},
            {L"One-time media",  &config.oneTimeMedia},
            {L"GIFs",            &config.animations},
            {L"Stickers",        &config.stickers},
        };

        float toggleY = 240;
        for (const auto & [name, value] : toggles) {
            exporter::Button toggle;
            toggle.text = std::wstring(name) + L": " + (*value ? L"true" : L"false");
            toggle.setFontSize(18);
            toggle.x = 40;
            toggle.y = toggleY;
            scene.uiButtons.push_back(std::move(toggle));

            const auto toggleIt = std::prev(scene.uiButtons.end());
            toggleIt->on_click = [toggleIt, name, value]() {
                *value = !*value;
                toggleIt->text = std::wstring(name) + L": " + (*value ? L"true" : L"false");
                exporter::saveConfig(exporter::g_client->getConfig(), exporter::appFilePath("config.json"));
            };

            toggleY += 26;
        }

        exporter::Button mediaSize;
        mediaSize.text = L"Max media size: " + std::to_wstring(config.maxMediaSizeMb) + L" MB";
        mediaSize.setFontSize(18);
        mediaSize.x = 40;
        mediaSize.y = toggleY;
        scene.uiButtons.push_back(std::move(mediaSize));

        const auto mediaSizeIt = std::prev(scene.uiButtons.end());
        mediaSizeIt->on_click = [mediaSizeIt, &config]() {
            static constexpr int steps[] = {256, 512, 1024, 2048, 4096};
            constexpr int stepCount = std::size(steps);

            int next = 0;
            for (int i = 0; i < stepCount; ++i) {
                if (steps[i] == config.maxMediaSizeMb) {
                    next = (i + 1) % stepCount;
                    break;
                }
            }

            config.maxMediaSizeMb = steps[next];
            mediaSizeIt->text = L"Max media size: " + std::to_wstring(config.maxMediaSizeMb) + L" MB";
            exporter::saveConfig(config, exporter::appFilePath("config.json"));
        };

        exporter::g_controlPanel->scenes.emplace(std::move(scene));
        exporter::Scene & topScene = exporter::g_controlPanel->scenes.top();
        topScene.uiButtonsIterator = topScene.uiButtons.begin();
        topScene.uiButtonsIterator->selected = true;
    }


    void uiClosePanel() {
        ShowWindow(g_controlPanel->hwnd_, SW_HIDE);
    }
}
