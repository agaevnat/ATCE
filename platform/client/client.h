#ifndef TELEGRAMCHATEXPORTER_CLIENT_H
#define TELEGRAMCHATEXPORTER_CLIENT_H
#include <windows.h>
#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <memory>
#include <string>
#include <atomic>
#include <list>
#include <map>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <nlohmann/json.hpp>

namespace exporter {
    struct Config {
        int apiId = 0;
        std::string apiHash;
        std::wstring exportPath;

        bool startByDefault = false;
        bool personalChats = true;
        bool groupChats = true;
        bool photos = true;
        bool videos = true;
        bool voices = true;
        bool circleMessages = true;
        bool files = true;
        bool oneTimeMedia = true;  // self-destructing photos/videos
        bool animations = true;    // GIFs
        bool stickers = true;
        int maxMediaSizeMb = 256;  // media bigger than this is not downloaded
    };

    struct AutoExportStats {
        std::list<long long> pendingChatIds;
        long long lastChatId = 0;
        size_t messagesWritten = 0;
    };

    std::string appFilePath(const std::string & fileName);

    Config loadConfig(const std::string & path);
    void saveConfig(const Config & config, const std::string & path);

    class client {
    private:
        std::unique_ptr<td::ClientManager> clientManager;
        int clientId = 0;
        bool authorized = false;
        std::uint64_t nextQueryId = 1;
        std::atomic<bool> stopRequested = false;
        std::atomic<bool> autoExportEnabled = false;
        Config config;

        std::map<std::uint64_t, td::td_api::object_ptr<td::td_api::Object>> pendingResponses;
        std::mutex responseMutex;
        std::condition_variable responseCv;
        std::map<long long, std::string> chatTitleCache;
        mutable std::mutex chatCacheMutex;
        mutable std::mutex autoStatsMutex;
        AutoExportStats autoStats;

        struct AutoUpdate {
            long long chatId = 0;
            long long messageId = 0;
            td::td_api::object_ptr<td::td_api::message> message;
            td::td_api::object_ptr<td::td_api::MessageContent> content;
        };

        std::list<AutoUpdate> autoQueue;
        mutable std::mutex autoQueueMutex;
        std::condition_variable autoQueueCv;
        std::map<long long, int> chatTypeCache;

        std::mutex exportFileMutex;

        bool isChatTypeEnabled(long long chatId);
        bool isMediaTypeEnabled(const nlohmann::json & msgJson) const;
        void writeMessageToExportFile(long long chatId, const nlohmann::json & msgJson, bool isEdit);

        // Merges fields into an already-exported message without touching
        // its edit history (used to fill in the media path after download).
        void patchExportedMessage(long long chatId, long long messageId, const nlohmann::json & fields);

        // Downloads the message's media (if any) through TDLib, copies it
        // next to the chat's JSON and records the path in it.
        void downloadMediaForMessage(long long chatId, const nlohmann::json & msgJson);

        td::td_api::object_ptr<td::td_api::Object> waitForResponse(std::uint64_t queryId);

    public:
        void listen();
        void stop() noexcept;
        bool auth();
        bool isAuthorized() const noexcept;
        Config & getConfig() noexcept;

        void runAutonomousExport();
        void startAutoExport() noexcept;
        void stopAutoExport();
        bool isAutoExportEnabled() const noexcept;

        AutoExportStats getAutoExportStats() const;
        std::wstring getChatTitleWide(long long chatId) const;
    };

    extern client * g_client;
}


#endif //TELEGRAMCHATEXPORTER_CLIENT_H
