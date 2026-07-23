#include "core/config.h"
#include "core/console.h"

#include <fstream>
#include <iostream>

namespace exporter {

    static std::string prompt_line(const std::string& prompt) {
        ensure_console_visible();
        std::cout << prompt << std::flush;
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    static std::int32_t prompt_api_id() {
        while (true) {
            auto line = prompt_line("api_id: ");
            try {
                return std::stoi(line);
            } catch (const std::exception&) {
                std::cout << "Похоже, это не число. Попробуйте ещё раз." << std::endl;
            }
        }
    }

    static nlohmann::json default_media() {
        return {
            {"photos", true},
            {"videos", true},
            {"voice_messages", true},
            {"files", true},
            {"max_file_size_mb", 200},
        };
    }

    static Config create_via_prompt(const std::string& path) {
        ensure_console_visible();
        std::cout << "Файл " << path << " не найден — похоже, это первый запуск.\n"
                     "api_id и api_hash можно получить бесплатно на https://my.telegram.org (My Applications)."
                  << std::endl;

        Config config;
        config.api_id = prompt_api_id();
        config.api_hash = prompt_line("api_hash: ");
        config.defaults = {
            {"mode", "interval"},
            {"interval_minutes", 360},
            {"media", default_media()},
        };
        config.chats = nlohmann::json::array();

        save(config, path);
        std::cout << "Создан " << path << " — остальные настройки можно поменять там вручную." << std::endl;
        return config;
    }

    Config load_or_create(const std::string& path) {
        std::ifstream in(path);
        if (!in.good()) {
            return create_via_prompt(path);
        }

        nlohmann::json j;
        in >> j;

        Config config;
        config.api_id = j.value("api_id", 0);
        config.api_hash = j.value("api_hash", std::string());
        config.export_path = j.value("export_path", std::string("./exports"));
        config.session_path = j.value("session_path", std::string("./session"));
        config.auto_discover_private_chats = j.value("auto_discover_private_chats", true);
        config.new_chat_default_action = j.value("new_chat_default_action", std::string("ask"));
        config.defaults = j.value("defaults", nlohmann::json::object());
        config.chats = j.value("chats", nlohmann::json::array());
        return config;
    }

    void save(const Config& config, const std::string& path) {
        nlohmann::json j;
        j["api_id"] = config.api_id;
        j["api_hash"] = config.api_hash;
        j["export_path"] = config.export_path;
        j["session_path"] = config.session_path;
        j["auto_discover_private_chats"] = config.auto_discover_private_chats;
        j["new_chat_default_action"] = config.new_chat_default_action;
        j["defaults"] = config.defaults;
        j["chats"] = config.chats;

        std::ofstream out(path);
        out << j.dump(2) << std::endl;
    }

}  // namespace exporter
