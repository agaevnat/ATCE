#ifndef TELEGRAMCHATEXPORTER_CLIENT_H
#define TELEGRAMCHATEXPORTER_CLIENT_H
#include <windows.h>
#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <memory>
#include <string>
#include <atomic>

namespace exporter {
    struct Config {
        int apiId = 0;
        std::string apiHash;
    };

    Config loadConfig(const std::string & path);

    class client {
    private:
        std::unique_ptr<td::ClientManager> clientManager;
        int clientId = 0;
        bool authorized = false;
        std::uint64_t nextQueryId = 1;  // 0 is reserved by TDLib for push updates
        std::atomic<bool> stopRequested = false;

    public:
        void listen();
        void stop() noexcept;
        bool auth();
        bool isAuthorized() const noexcept;
    };
}


#endif //TELEGRAMCHATEXPORTER_CLIENT_H
