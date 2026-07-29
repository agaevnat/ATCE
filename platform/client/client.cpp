#include "client.h"
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <ctime>
#include <nlohmann/json.hpp>


static bool g_consoleAllocated = false;

static BOOL WINAPI consoleCtrlHandler(const DWORD ctrlType) {
    if (ctrlType == CTRL_CLOSE_EVENT) {
        ExitProcess(1);
    }
    return FALSE;
}

static void ensureConsole() {
    if (g_consoleAllocated) {
        return;
    }
    g_consoleAllocated = true;

    AllocConsole();
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);

    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONIN$", "r", stdin);
}

static std::string prompt(const std::string & message) {
    ensureConsole();
    std::cout << message;
    std::string line;
    std::getline(std::cin, line);
    return line;
}


static std::wstring toWide(const std::string & utf8) {
    if (utf8.empty()) {
        return {};
    }

    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), wide.data(), size);
    return wide;
}

static std::string toUtf8(const std::wstring & wide) {
    if (wide.empty()) {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string utf8(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), utf8.data(), size, nullptr, nullptr);
    return utf8;
}


static void applyContentToJson(td::td_api::MessageContent & content, nlohmann::json & msgJson) {
    msgJson["text"] = "";
    msgJson["media_type"] = nullptr;

    const auto contentId = content.get_id();
    if (contentId == td::td_api::messageText::ID) {
        msgJson["text"] = static_cast<td::td_api::messageText &>(content).text_->text_;
    } else if (contentId == td::td_api::messagePhoto::ID) {
        auto & photo = static_cast<td::td_api::messagePhoto &>(content);
        msgJson["text"] = photo.caption_->text_;
        msgJson["media_type"] = "photo";
        msgJson["is_secret"] = photo.is_secret_;
        if (!photo.photo_->sizes_.empty()) {
            auto & bestSize = photo.photo_->sizes_.back();
            msgJson["file_id"] = bestSize->photo_->id_;
            msgJson["file_size"] = bestSize->photo_->size_;
            msgJson["downloaded"] = false;
        }
    } else if (contentId == td::td_api::messageVideo::ID) {
        auto & video = static_cast<td::td_api::messageVideo &>(content);
        msgJson["text"] = video.caption_->text_;
        msgJson["media_type"] = "video";
        msgJson["is_secret"] = video.is_secret_;
        msgJson["file_id"] = video.video_->video_->id_;
        msgJson["file_size"] = video.video_->video_->size_;
        msgJson["downloaded"] = false;
    } else if (contentId == td::td_api::messageVideoNote::ID) {
        auto & videoNote = static_cast<td::td_api::messageVideoNote &>(content);
        msgJson["media_type"] = "video_note";
        msgJson["file_id"] = videoNote.video_note_->video_->id_;
        msgJson["file_size"] = videoNote.video_note_->video_->size_;
        msgJson["downloaded"] = false;
    } else if (contentId == td::td_api::messageDocument::ID) {
        auto & document = static_cast<td::td_api::messageDocument &>(content);
        msgJson["text"] = document.caption_->text_;
        msgJson["media_type"] = "document";
        msgJson["file_id"] = document.document_->document_->id_;
        msgJson["file_size"] = document.document_->document_->size_;
        msgJson["downloaded"] = false;
    } else if (contentId == td::td_api::messageVoiceNote::ID) {
        auto & voiceNote = static_cast<td::td_api::messageVoiceNote &>(content);
        msgJson["text"] = voiceNote.caption_->text_;
        msgJson["media_type"] = "voice_note";
        msgJson["file_id"] = voiceNote.voice_note_->voice_->id_;
        msgJson["file_size"] = voiceNote.voice_note_->voice_->size_;
        msgJson["downloaded"] = false;
    } else if (contentId == td::td_api::messageAnimation::ID) {
        auto & animation = static_cast<td::td_api::messageAnimation &>(content);
        msgJson["text"] = animation.caption_->text_;
        msgJson["media_type"] = "animation";
        msgJson["file_id"] = animation.animation_->animation_->id_;
        msgJson["file_size"] = animation.animation_->animation_->size_;
        msgJson["downloaded"] = false;
    } else if (contentId == td::td_api::messageSticker::ID) {
        auto & sticker = static_cast<td::td_api::messageSticker &>(content);
        msgJson["media_type"] = "sticker";
        msgJson["file_id"] = sticker.sticker_->sticker_->id_;
        msgJson["file_size"] = sticker.sticker_->sticker_->size_;
        msgJson["downloaded"] = false;
    } else {
        msgJson["media_type"] = "other";
    }
}


static nlohmann::json messageToJson(td::td_api::message & message) {
    nlohmann::json msgJson;
    msgJson["id"] = message.id_;
    msgJson["date"] = message.date_;

    if (message.sender_id_->get_id() == td::td_api::messageSenderUser::ID) {
        msgJson["sender_id"] = static_cast<td::td_api::messageSenderUser &>(*message.sender_id_).user_id_;
        msgJson["sender_type"] = "user";
    } else if (message.sender_id_->get_id() == td::td_api::messageSenderChat::ID) {
        msgJson["sender_id"] = static_cast<td::td_api::messageSenderChat &>(*message.sender_id_).chat_id_;
        msgJson["sender_type"] = "chat";
    }

    applyContentToJson(*message.content_, msgJson);
    return msgJson;
}



namespace exporter {
    std::string appFilePath(const std::string & fileName) {
        wchar_t exePath[MAX_PATH];
        const DWORD length = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        if (length == 0) {
            return "./" + fileName;
        }

        std::wstring directory(exePath, length);
        const auto lastSlash = directory.find_last_of(L'\\');
        if (lastSlash != std::wstring::npos) {
            directory.erase(lastSlash + 1);
        }

        return toUtf8(directory) + fileName;
    }

    Config loadConfig(const std::string & path) {
        std::ifstream in(path);

        if (in.good()) {
            nlohmann::json j;
            in >> j;

            Config config;
            config.apiId = j.value("api_id", 0);
            config.apiHash = j.value("api_hash", std::string());
            config.exportPath = toWide(j.value("export_path", std::string()));
            config.startByDefault = j.value("start_by_default", false);
            config.personalChats = j.value("personal_chats", true);
            config.groupChats = j.value("group_chats", true);
            config.botChats = j.value("bot_chats", true);
            config.photos = j.value("photos", true);
            config.videos = j.value("videos", true);
            config.voices = j.value("voices", true);
            config.circleMessages = j.value("circle_messages", true);
            config.files = j.value("files", true);
            config.oneTimeMedia = j.value("one_time_media", true);
            config.animations = j.value("animations", true);
            config.stickers = j.value("stickers", true);
            config.maxMediaSizeMb = j.value("max_media_size_mb", 256);
            return config;
        }

        ensureConsole();
        std::cout << "File " << path << " not found. Looks like this is a first launch.\n"
                     "api_id and api_hash can be obtained at https://my.telegram.org (My Applications)."
                  << std::endl;

        Config config;
        config.apiId = std::stoi(prompt("api_id: "));
        config.apiHash = prompt("api_hash: ");

        saveConfig(config, path);

        return config;
    }

    void saveConfig(const Config & config, const std::string & path) {
        nlohmann::json j;
        j["api_id"] = config.apiId;
        j["api_hash"] = config.apiHash;
        j["export_path"] = toUtf8(config.exportPath);
        j["start_by_default"] = config.startByDefault;
        j["personal_chats"] = config.personalChats;
        j["group_chats"] = config.groupChats;
        j["bot_chats"] = config.botChats;
        j["photos"] = config.photos;
        j["videos"] = config.videos;
        j["voices"] = config.voices;
        j["circle_messages"] = config.circleMessages;
        j["files"] = config.files;
        j["one_time_media"] = config.oneTimeMedia;
        j["animations"] = config.animations;
        j["stickers"] = config.stickers;
        j["max_media_size_mb"] = config.maxMediaSizeMb;
        std::ofstream out(path);
        out << j.dump(2) << std::endl;
    }

    
    bool client::auth() {
        this->config = loadConfig(appFilePath("config.json"));
        Config & config = this->config;
        this->autoExportEnabled = config.startByDefault;
        this->clientManager = std::make_unique<td::ClientManager>();
        this->clientId = this->clientManager->create_client_id();
        this->clientManager->send(this->clientId, this->nextQueryId++, td::td_api::make_object<td::td_api::getOption>("version"));

        while (!this->authorized) {
            auto response = this->clientManager->receive(10.0);
            if (!response.object) {
                continue;
            }

            if (response.object->get_id() != td::td_api::updateAuthorizationState::ID) {
                continue;
            }

            auto & update = static_cast<td::td_api::updateAuthorizationState &>(*response.object);
            const int stateId = update.authorization_state_->get_id();

            if (stateId == td::td_api::authorizationStateWaitTdlibParameters::ID) {
                ensureConsole();
                this->clientManager->send(this->clientId, this->nextQueryId++, td::td_api::make_object<td::td_api::setTdlibParameters>(
                    false,                     // use_test_dc
                    appFilePath("session"),    // database_directory
                    "",                        // files_directory
                    "",                        // database_encryption_key
                    true,                      // use_file_database
                    true,                      // use_chat_info_database
                    true,                      // use_message_database
                    true,                      // use_secret_chats
                    config.apiId,              // api_id
                    config.apiHash,            // api_hash
                    "ru",                      // system_language_code
                    "TelegramChatExporter",    // device_model
                    "",                        // system_version
                    "0.1"                      // application_version
                ));
            }
            else if (stateId == td::td_api::authorizationStateWaitPhoneNumber::ID) {
                std::string phone = prompt("Phone number (international format eg. +79991234567): ");
                this->clientManager->send(this->clientId, this->nextQueryId++, td::td_api::make_object<td::td_api::setAuthenticationPhoneNumber>(phone, nullptr));
            }
            else if (stateId == td::td_api::authorizationStateWaitCode::ID) {
                std::string code = prompt("Code from Telegram: ");
                this->clientManager->send(this->clientId, this->nextQueryId++, td::td_api::make_object<td::td_api::checkAuthenticationCode>(code));
            }
            else if (stateId == td::td_api::authorizationStateWaitPassword::ID) {
                std::string password = prompt("Two-factor authentication password: ");
                this->clientManager->send(this->clientId, this->nextQueryId++, td::td_api::make_object<td::td_api::checkAuthenticationPassword>(password));
            }
            else if (stateId == td::td_api::authorizationStateReady::ID) {
                this->authorized = true;
                if (g_consoleAllocated) {
                    std::cout << "Authorized! You can access the control panel from the tray icon.\n"
                                 "This window will be closed in 5 seconds...\nDo not close the window yourself or the program will terminate!" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    g_consoleAllocated = false;
                    ShowWindow(GetConsoleWindow(), SW_HIDE);
                    FreeConsole();
                }
            }
        }

        return this->authorized;
    }

    bool client::isAuthorized() const noexcept {
        return this->authorized;
    }

    Config & client::getConfig() noexcept {
        return this->config;
    }


    void client::listen() {
        while (!this->stopRequested) {
            auto response = this->clientManager->receive(10.0);
            if (!response.object) {
                continue;
            }

            if (response.request_id != 0) {
                std::lock_guard lock(this->responseMutex);
                this->pendingResponses[response.request_id] = std::move(response.object);
                this->responseCv.notify_all();
                continue;
            }
            if (!this->autoExportEnabled) {
                continue;
            }

            const auto updateId = response.object->get_id();
            if (updateId == td::td_api::updateNewMessage::ID) {
                auto & update = static_cast<td::td_api::updateNewMessage &>(*response.object);

                AutoUpdate autoUpdate;
                autoUpdate.chatId = update.message_->chat_id_;
                autoUpdate.messageId = update.message_->id_;
                autoUpdate.message = std::move(update.message_);

                std::lock_guard lock(this->autoQueueMutex);
                this->autoQueue.push_back(std::move(autoUpdate));
                this->autoQueueCv.notify_one();
            }
            else if (updateId == td::td_api::updateMessageContent::ID) {
                auto & update = static_cast<td::td_api::updateMessageContent &>(*response.object);

                AutoUpdate autoUpdate;
                autoUpdate.chatId = update.chat_id_;
                autoUpdate.messageId = update.message_id_;
                autoUpdate.content = std::move(update.new_content_);

                std::lock_guard lock(this->autoQueueMutex);
                this->autoQueue.push_back(std::move(autoUpdate));
                this->autoQueueCv.notify_one();
            }
        }
        this->autoQueueCv.notify_all();
    }

    void client::stop() noexcept {
        this->stopRequested = true;
        this->autoQueueCv.notify_all();
        this->responseCv.notify_all();
    }

    td::td_api::object_ptr<td::td_api::Object> client::waitForResponse(const std::uint64_t queryId) {
        std::unique_lock lock(this->responseMutex);
        this->responseCv.wait(lock, [this, queryId] {
            return this->stopRequested || this->pendingResponses.count(queryId) > 0;
        });

        if (this->pendingResponses.count(queryId) == 0) {
            return td::td_api::make_object<td::td_api::error>(0, "client is shutting down");
        }

        auto object = std::move(pendingResponses[queryId]);
        this->pendingResponses.erase(queryId);
        return object;
    }

    bool client::isChatTypeEnabled(const long long chatId) {
        ChatKind kind = ChatKind::Group;
        bool haveCached = false;
        {
            std::lock_guard<std::mutex> lock(this->chatCacheMutex);
            const auto cached = this->chatTypeCache.find(chatId);
            if (cached != this->chatTypeCache.end()) {
                kind = cached->second;
                haveCached = true;
            }
        }

        if (!haveCached) {
            const std::uint64_t queryId = this->nextQueryId++;
            this->clientManager->send(this->clientId, queryId, td::td_api::make_object<td::td_api::getChat>(chatId));
            auto response = waitForResponse(queryId);

            if (response->get_id() != td::td_api::chat::ID) {
                return false;
            }

            auto & chat = static_cast<td::td_api::chat &>(*response);
            const auto typeId = chat.type_->get_id();

            if (typeId == td::td_api::chatTypePrivate::ID || typeId == td::td_api::chatTypeSecret::ID) {
                // A bot conversation is a private chat as well — the only way
                // to tell is to look up the user on the other side.
                const auto userId = (typeId == td::td_api::chatTypePrivate::ID)
                    ? static_cast<td::td_api::chatTypePrivate &>(*chat.type_).user_id_
                    : static_cast<td::td_api::chatTypeSecret &>(*chat.type_).user_id_;

                kind = ChatKind::Personal;

                const std::uint64_t userQueryId = this->nextQueryId++;
                this->clientManager->send(this->clientId, userQueryId, td::td_api::make_object<td::td_api::getUser>(userId));
                auto userResponse = waitForResponse(userQueryId);

                if (userResponse->get_id() == td::td_api::user::ID) {
                    auto & user = static_cast<td::td_api::user &>(*userResponse);
                    if (user.type_ && user.type_->get_id() == td::td_api::userTypeBot::ID) {
                        kind = ChatKind::Bot;
                    }
                }
            } else {
                kind = ChatKind::Group;
            }

            std::lock_guard<std::mutex> lock(this->chatCacheMutex);
            this->chatTypeCache[chatId] = kind;
            this->chatTitleCache[chatId] = chat.title_;
        }

        switch (kind) {
            case ChatKind::Personal: return this->config.personalChats;
            case ChatKind::Bot:      return this->config.botChats;
            default:                 return this->config.groupChats;
        }
    }

    bool client::isMediaTypeEnabled(const nlohmann::json & msgJson) const {
        if (msgJson["media_type"].is_null()) {
            return true;  // plain text
        }

        if (msgJson.value("is_secret", false) && !this->config.oneTimeMedia) {
            return false;
        }

        const std::string mediaType = msgJson["media_type"].get<std::string>();
        if (mediaType == "photo")      return this->config.photos;
        if (mediaType == "video")      return this->config.videos;
        if (mediaType == "voice_note") return this->config.voices;
        if (mediaType == "video_note") return this->config.circleMessages;
        if (mediaType == "document")   return this->config.files;
        if (mediaType == "animation")  return this->config.animations;
        if (mediaType == "sticker")    return this->config.stickers;
        return true;
    }

    void client::writeMessageToExportFile(const long long chatId, const nlohmann::json & msgJson, const bool isEdit) {
        std::lock_guard<std::mutex> lock(this->exportFileMutex);
        const std::wstring fileName = this->config.exportPath + L"\\" + std::to_wstring(chatId) + L".json";

        nlohmann::json chatJson;
        std::ifstream in(fileName);
        if (in.good()) {
            in >> chatJson;
        }
        in.close();

        if (!chatJson.contains("messages") || !chatJson["messages"].is_array()) {
            chatJson["chat_id"] = chatId;
            chatJson["messages"] = nlohmann::json::array();
        }
        if (!chatJson.contains("edit_log") || !chatJson["edit_log"].is_array()) {
            chatJson["edit_log"] = nlohmann::json::array();
        }

        const long long messageId = msgJson["id"].get<long long>();
        bool replaced = false;
        for (auto & existing : chatJson["messages"]) {
            if (existing.contains("id") && existing["id"].get<long long>() == messageId) {
                if (isEdit) {
                    if (!existing.contains("edits") || !existing["edits"].is_array()) {
                        existing["edits"] = nlohmann::json::array();
                    }

                    nlohmann::json editRecord;
                    editRecord["edited_at"] = static_cast<long long>(std::time(nullptr));
                    editRecord["previous_text"] = existing.value("text", std::string());
                    editRecord["previous_media_type"] = existing.value("media_type", nlohmann::json());
                    editRecord["new_text"] = msgJson.value("text", std::string());
                    existing["edits"].push_back(editRecord);

                    editRecord["message_id"] = messageId;
                    chatJson["edit_log"].push_back(std::move(editRecord));

                    existing.update(msgJson);
                } else {
                    existing = msgJson;
                }
                replaced = true;
                break;
            }
        }

        if (!replaced) {
            if (isEdit) {
                return;
            }
            chatJson["messages"].push_back(msgJson);
        }

        std::ofstream out(fileName);
        out << chatJson.dump(2);
    }

    void client::patchExportedMessage(const long long chatId, const long long messageId, const nlohmann::json & fields) {
        std::lock_guard<std::mutex> lock(this->exportFileMutex);
        const std::wstring fileName = this->config.exportPath + L"\\" + std::to_wstring(chatId) + L".json";

        nlohmann::json chatJson;
        std::ifstream in(fileName);
        if (!in.good()) {
            return;
        }
        in >> chatJson;
        in.close();

        if (!chatJson.contains("messages") || !chatJson["messages"].is_array()) {
            return;
        }

        for (auto & existing : chatJson["messages"]) {
            if (existing.contains("id") && existing["id"].get<long long>() == messageId) {
                existing.update(fields);

                std::ofstream out(fileName);
                out << chatJson.dump(2);
                return;
            }
        }
    }

    void client::downloadMediaForMessage(const long long chatId, const nlohmann::json & msgJson) {
        if (!msgJson.contains("file_id")) {
            return;
        }

        if (msgJson.contains("file_size")) {
            const auto fileSize = msgJson["file_size"].get<long long>();
            const long long limitBytes = static_cast<long long>(this->config.maxMediaSizeMb) * 1024 * 1024;
            if (fileSize > limitBytes) {
                return;
            }
        }

        const auto fileId = msgJson["file_id"].get<std::int32_t>();
        const std::uint64_t queryId = this->nextQueryId++;
        this->clientManager->send(this->clientId, queryId, td::td_api::make_object<td::td_api::downloadFile>(fileId, 1, 0, 0, true));
        auto response = waitForResponse(queryId);

        if (response->get_id() != td::td_api::file::ID) {
            return;
        }

        auto & file = static_cast<td::td_api::file &>(*response);
        if (!file.local_ || !file.local_->is_downloading_completed_ || file.local_->path_.empty()) {
            return;
        }

        const std::wstring sourcePath = toWide(file.local_->path_);
        const auto lastSlash = sourcePath.find_last_of(L'\\');
        const std::wstring baseName = (lastSlash == std::wstring::npos) ? sourcePath : sourcePath.substr(lastSlash + 1);

        const std::wstring mediaDir = this->config.exportPath + L"\\" + std::to_wstring(chatId) + L"_files";
        CreateDirectoryW(mediaDir.c_str(), nullptr);

        const std::wstring targetPath = mediaDir + L"\\" + baseName;
        if (!CopyFileW(sourcePath.c_str(), targetPath.c_str(), FALSE)) {
            return;
        }

        nlohmann::json fields;
        fields["downloaded"] = true;
        fields["file_path"] = toUtf8(std::to_wstring(chatId) + L"_files\\" + baseName);
        patchExportedMessage(chatId, msgJson["id"].get<long long>(), fields);
    }

    void client::runAutonomousExport() {
        while (!this->stopRequested) {
            AutoUpdate update;
            {
                std::unique_lock<std::mutex> lock(this->autoQueueMutex);
                this->autoQueueCv.wait(lock, [this] { return this->stopRequested || !this->autoQueue.empty(); });

                if (this->stopRequested) {
                    return;
                }

                update = std::move(this->autoQueue.front());
                this->autoQueue.pop_front();
            }

            if (!this->autoExportEnabled || this->config.exportPath.empty() || !isChatTypeEnabled(update.chatId)) {
                continue;
            }

            nlohmann::json msgJson;
            bool isEdit = false;

            if (update.message) {
                msgJson = messageToJson(*update.message);
            } else if (update.content) {
                msgJson["id"] = update.messageId;
                applyContentToJson(*update.content, msgJson);
                isEdit = true;
            } else {
                continue;
            }

            if (!isMediaTypeEnabled(msgJson)) {
                continue;
            }

            // Record the message first: if the download stalls or fails, we
            // still keep the fact that it was sent.
            writeMessageToExportFile(update.chatId, msgJson, isEdit);
            downloadMediaForMessage(update.chatId, msgJson);

            std::lock_guard<std::mutex> lock(this->autoStatsMutex);
            this->autoStats.lastChatId = update.chatId;
            ++this->autoStats.messagesWritten;
        }
    }

    void client::startAutoExport() noexcept {
        this->autoExportEnabled = true;
    }

    void client::stopAutoExport() {
        this->autoExportEnabled = false;
        std::lock_guard<std::mutex> lock(this->autoQueueMutex);
        this->autoQueue.clear();
    }

    bool client::isAutoExportEnabled() const noexcept {
        return this->autoExportEnabled;
    }

    AutoExportStats client::getAutoExportStats() const {
        AutoExportStats stats;
        {
            std::lock_guard<std::mutex> lock(this->autoStatsMutex);
            stats = this->autoStats;
        }

        std::lock_guard<std::mutex> queueLock(this->autoQueueMutex);
        for (const auto & update : this->autoQueue) {
            stats.pendingChatIds.push_back(update.chatId);
        }
        return stats;
    }

    std::wstring client::getChatTitleWide(const long long chatId) const {
        std::lock_guard<std::mutex> lock(this->chatCacheMutex);
        const auto it = this->chatTitleCache.find(chatId);
        if (it == this->chatTitleCache.end()) {
            return std::to_wstring(chatId);
        }
        return toWide(it->second);
    }

    client * g_client = nullptr;
}
