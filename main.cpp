#ifdef _WIN32
#include "platform/windows/tray.h"
#include "platform/windows/control_panel.h"
#endif
#include <utility>

#ifdef _WIN32

int main() {
    exporter::TrayMenu trayMenu;
    exporter::ControlPanel controlPanel;

    const std::array<exporter::TrayMenuItem, exporter::trayMenuOptionNum> items = {{
        {L"Open Control Panel", [&controlPanel] {controlPanel.run();}},
        {L"Terminate", [&trayMenu]{trayMenu.terminate();}}
    }};

    return trayMenu.run(L"TelegramChatExporter", items);
}

#else

int main() {
    return 0;
}

#endif
