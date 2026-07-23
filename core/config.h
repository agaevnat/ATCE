#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

namespace exporter {

struct Config {
    std::int32_t api_id = 0;
    std::string api_hash;
    std::string export_path = "./exports";
    std::string session_path = "./session";
    bool auto_discover_private_chats = true;
    std::string new_chat_default_action = "ask";

    // Not consumed anywhere yet (core/chat_exporter and core/scheduler
    // aren't built) — kept as raw JSON so hand-edited or future fields
    // round-trip through load/save without being modeled twice.
    nlohmann::json defaults;
    nlohmann::json chats;
};

// Loads config.json from `path`. If the file doesn't exist, prompts on
// stdin for api_id/api_hash (the only fields with no sane default) and
// writes a new config.json with defaults for everything else.
Config load_or_create(const std::string& path);

void save(const Config& config, const std::string& path);

}  // namespace exporter
