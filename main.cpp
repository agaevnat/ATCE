#ifdef _WIN32
#include "platform/windows/tray.h"
#include "platform/windows/control_panel.h"
#endif
#include "platform/client/client.h"
#include <utility>
#include <thread>
#include <mutex>
#include <condition_variable>


#ifdef _WIN32

int main() {
    exporter::client client;

    std::mutex authMutex;
    std::condition_variable authCv;
    bool authDone = false;

    std::thread authThread([&] {
        client.auth();
        {
            std::lock_guard<std::mutex> lock(authMutex);
            authDone = true;
        }
        authCv.notify_one();
    });

    {
        std::unique_lock<std::mutex> lock(authMutex);
        authCv.wait(lock, [&] { return authDone; });
    }
    authThread.join();

    if (!client.isAuthorized()) {
        return 1;
    }

    exporter::g_client = &client;
    std::thread listenThread(&exporter::client::listen, &client);
    std::thread autoExportThread(&exporter::client::runAutonomousExport, &client);

    exporter::TrayMenu trayMenu;
    exporter::ControlPanel controlPanel;

    const std::array<exporter::TrayMenuItem, exporter::trayMenuOptionNum> items = {{
        {L"Open Control Panel", [&controlPanel] {controlPanel.run();}},
        {L"Terminate", [&trayMenu]{trayMenu.terminate();}}
    }};

    const int exitCode = trayMenu.run(L"ATCE", items);

    client.stop();
    listenThread.join();
    autoExportThread.join();

    return exitCode;
}

#else

int main() {
    return 0;
}

#endif
