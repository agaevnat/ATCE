#include "client.h"
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
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


namespace exporter {
    Config loadConfig(const std::string & path) {
        std::ifstream in(path);

        if (in.good()) {
            nlohmann::json j;
            in >> j;

            Config config;
            config.apiId = j.value("api_id", 0);
            config.apiHash = j.value("api_hash", std::string());
            return config;
        }

        ensureConsole();
        std::cout << "File " << path << " not found. Looks like this is a first launch.\n"
                     "api_id and api_hash can be obtained at https://my.telegram.org (My Applications)."
                  << std::endl;

        Config config;
        config.apiId = std::stoi(prompt("api_id: "));
        config.apiHash = prompt("api_hash: ");

        nlohmann::json j;
        j["api_id"] = config.apiId;
        j["api_hash"] = config.apiHash;
        std::ofstream out(path);
        out << j.dump(2) << std::endl;

        return config;
    }

    
    bool client::auth() {
        Config config = loadConfig("./config.json");

        this->clientManager = std::make_unique<td::ClientManager>();
        this->clientId = this->clientManager->create_client_id();

        // TDLib doesn't actually start the client's state machine (and so
        // never sends the first authorizationState update) until it gets
        // at least one send() for this clientId — this "kicks" it awake.
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
                clientManager->send(clientId, this->nextQueryId++, td::td_api::make_object<td::td_api::setTdlibParameters>(
                    false,                     // use_test_dc
                    "./session",               // database_directory
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
                clientManager->send(clientId, this->nextQueryId++, td::td_api::make_object<td::td_api::setAuthenticationPhoneNumber>(phone, nullptr));
            }
            else if (stateId == td::td_api::authorizationStateWaitCode::ID) {
                std::string code = prompt("Code from Telegram: ");
                clientManager->send(clientId, this->nextQueryId++, td::td_api::make_object<td::td_api::checkAuthenticationCode>(code));
            }
            else if (stateId == td::td_api::authorizationStateWaitPassword::ID) {
                std::string password = prompt("Two-factor authentication password: ");
                clientManager->send(clientId, this->nextQueryId++, td::td_api::make_object<td::td_api::checkAuthenticationPassword>(password));
            }
            else if (stateId == td::td_api::authorizationStateReady::ID) {
                authorized = true;
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

        return authorized;
    }

    bool client::isAuthorized() const noexcept {
        return authorized;
    }


    void client::listen() {
        while (!stopRequested) {
            const auto response = this->clientManager->receive(10.0);
            if (!response.object) {
                continue;
            }
        }
    }

    void client::stop() noexcept {
        stopRequested = true;
    }
}
