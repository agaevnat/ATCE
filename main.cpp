#include "core/auth.h"
#include "core/config.h"
#include "core/console.h"
#include "core/tdlib_client.h"

#ifdef _WIN32
#include "platform/windows/windows_agent.h"
#endif

#include <atomic>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

namespace {

void run_backend(std::atomic<bool>& ready) {
    auto app_config = exporter::load_or_create("./config.json");

    exporter::AuthConfig auth_config;
    auth_config.api_id = app_config.api_id;
    auth_config.api_hash = app_config.api_hash;
    auth_config.session_path = app_config.session_path;

    exporter::TdlibClient client;
    exporter::Auth auth(client, auth_config);
    auth.wait_until_ready();

    if (!auth.is_ready()) {
        return;
    }
    ready = true;
    exporter::hide_console();

    // Placeholder run loop: keeps the TDLib connection (and the process)
    // alive until core/chat_exporter + core/scheduler replace it with the
    // real per-chat export/sync work.
    while (true) {
        client.poll(10.0);
    }
}

std::string read_config_text() {
    std::ifstream in("./config.json");
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

}  // namespace

#ifdef _WIN32

int main() {
    if (exporter::is_agent_running()) {
        exporter::show_message_box(L"TelegramChatExporter", L"Программа уже запущена — смотрите иконку в трее.");
        return 0;
    }
    exporter::mark_agent_running();

    std::atomic<bool> ready{false};
    std::thread backend_thread(run_backend, std::ref(ready));
    backend_thread.detach();

    std::vector<exporter::TrayMenuItem> items = {
        {L"Статус",
         [&ready] {
             exporter::show_message_box(
                 L"Статус", ready ? L"Авторизован, работает в фоне." : L"Идёт настройка/авторизация...");
         }},
        {L"Показать конфиг", [] { exporter::show_message_box(L"config.json", exporter::utf8_to_wide(read_config_text())); }},
        {L"Выход", [] { exporter::quit_tray_icon(); }},
    };

    return exporter::run_tray_icon(L"TelegramChatExporter", std::move(items));
}

#else

int main() {
    std::atomic<bool> ready{false};
    run_backend(ready);
    return 0;
}

#endif
