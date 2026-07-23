#include "core/auth.h"
#include "core/config.h"
#include "core/console.h"
#include "core/tdlib_client.h"

#ifdef _WIN32
#include "platform/windows/menu_window.h"
#include "platform/windows/windows_agent.h"
#endif

#include <atomic>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iterator>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

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

#ifdef _WIN32

constexpr const wchar_t* kBorder = L"────────────────────────────────────────────";

enum class Screen { Main, Config };

Screen g_screen = Screen::Main;
int g_selected_field = 0;  // 0 = nothing selected
exporter::Config g_config;
bool g_config_loaded = false;

constexpr int kIntervalSteps[] = {5, 10, 20, 40, 60, 90, 120};
constexpr int kFileSizeSteps[] = {64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};

int next_in_carousel(int current, const int* steps, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (steps[i] == current) {
            return steps[(i + 1) % count];
        }
    }
    // Doesn't match a step exactly (e.g. hand-edited config.json) — snap to
    // the next one up, wrapping around if it's past the last step.
    for (size_t i = 0; i < count; ++i) {
        if (steps[i] > current) {
            return steps[i];
        }
    }
    return steps[0];
}

void ensure_config_loaded() {
    if (!g_config_loaded) {
        g_config = exporter::load_or_create("./config.json");
        g_config_loaded = true;
    }
}

void save_config() {
    exporter::save(g_config, "./config.json");
}

struct EditableField {
    std::wstring label;
    std::function<std::wstring()> display_value;
    std::function<void()> activate;  // called on Enter
};

std::vector<EditableField> build_editable_fields() {
    std::vector<EditableField> fields;

    auto add_bool_field = [&fields](const std::wstring& label, std::string key) {
        fields.push_back({label,
                          [key]() -> std::wstring {
                              bool value = g_config.defaults["media"].value(key, true);
                              return value ? L"Включено" : L"Выключено";
                          },
                          [key]() {
                              bool current = g_config.defaults["media"].value(key, true);
                              g_config.defaults["media"][key] = !current;
                              save_config();
                          }});
    };

    add_bool_field(L"Voice Messages", "voice_messages");
    add_bool_field(L"Photos", "photos");
    add_bool_field(L"Videos", "videos");
    add_bool_field(L"Files", "files");

    fields.push_back({L"Интервал (минуты)",
                      []() -> std::wstring { return std::to_wstring(g_config.defaults.value("interval_minutes", 360)); },
                      []() {
                          int current = g_config.defaults.value("interval_minutes", 360);
                          g_config.defaults["interval_minutes"] =
                              next_in_carousel(current, kIntervalSteps, std::size(kIntervalSteps));
                          save_config();
                      }});

    fields.push_back(
        {L"Макс. размер файла (МБ)",
         []() -> std::wstring { return std::to_wstring(g_config.defaults["media"].value("max_file_size_mb", 200)); },
         []() {
             int current = g_config.defaults["media"].value("max_file_size_mb", 200);
             g_config.defaults["media"]["max_file_size_mb"] =
                 next_in_carousel(current, kFileSizeSteps, std::size(kFileSizeSteps));
             save_config();
         }});

    return fields;
}

void render_config_screen() {
    ensure_config_loaded();
    exporter::menu_clear();

    exporter::menu_println(L"");
    exporter::menu_println(L"              Конфигурация", exporter::kMenuColorTitle);
    exporter::menu_println(kBorder, exporter::kMenuColorBorder);
    exporter::menu_println(L"");

    exporter::menu_println(L"  api_id: " + std::to_wstring(g_config.api_id));
    exporter::menu_println(L"  api_hash: " + exporter::utf8_to_wide(g_config.api_hash));
    exporter::menu_println(L"  export_path: " + exporter::utf8_to_wide(g_config.export_path));
    exporter::menu_println(L"  session_path: " + exporter::utf8_to_wide(g_config.session_path));
    exporter::menu_println(L"  auto_discover_private_chats: " +
                           std::wstring(g_config.auto_discover_private_chats ? L"true" : L"false"));
    exporter::menu_println(L"  new_chat_default_action: " + exporter::utf8_to_wide(g_config.new_chat_default_action));
    exporter::menu_println(L"  mode: " + exporter::utf8_to_wide(g_config.defaults.value("mode", std::string("interval"))));
    exporter::menu_println(L"");

    auto fields = build_editable_fields();
    for (size_t i = 0; i < fields.size(); ++i) {
        bool selected = g_selected_field == static_cast<int>(i + 1);
        std::wstring line =
            L"  [" + std::to_wstring(i + 1) + L"] " + fields[i].label + L": " + fields[i].display_value();
        exporter::menu_println(line, exporter::kMenuColorOption, selected);
    }

    exporter::menu_println(L"");
    exporter::menu_println(kBorder, exporter::kMenuColorBorder);
    exporter::menu_println(L"  [0] Назад", exporter::kMenuColorOption);
    exporter::menu_println(L"");
}

void render_main_screen(std::atomic<bool>& ready) {
    exporter::menu_clear();

    exporter::menu_println(L"");
    exporter::menu_println(L"           TelegramChatExporter", exporter::kMenuColorTitle);
    exporter::menu_println(kBorder, exporter::kMenuColorBorder);
    exporter::menu_println(L"");
    exporter::menu_println(L"  [1] Показать конфиг", exporter::kMenuColorOption);
    exporter::menu_println(L"  [0] Скрыть окно", exporter::kMenuColorOption);
    exporter::menu_println(L"");
    exporter::menu_println(kBorder, exporter::kMenuColorBorder);
    exporter::menu_println(L"");
    if (ready) {
        exporter::menu_println(L"  ● Работает", exporter::kMenuColorGood);
    } else {
        exporter::menu_println(L"  ● Не запущен", exporter::kMenuColorBad);
    }
}

void handle_menu_key(wchar_t key, std::atomic<bool>& ready) {
    if (g_screen == Screen::Main) {
        if (key == L'1') {
            g_screen = Screen::Config;
            g_selected_field = 0;
            render_config_screen();
        } else if (key == L'0') {
            exporter::hide_menu_window();
        }
        return;
    }

    // Screen::Config
    if (key == L'0') {
        g_screen = Screen::Main;
        g_selected_field = 0;
        render_main_screen(ready);
        return;
    }

    auto fields = build_editable_fields();

    if (key >= L'1' && key <= L'9') {
        int index = key - L'0';
        if (index >= 1 && index <= static_cast<int>(fields.size())) {
            // Pressing the already-selected field's number again deselects it.
            g_selected_field = (g_selected_field == index) ? 0 : index;
            render_config_screen();
        }
        return;
    }

    if (key == L'\r' && g_selected_field >= 1 && g_selected_field <= static_cast<int>(fields.size())) {
        fields[g_selected_field - 1].activate();
        render_config_screen();
    }
}

void open_menu(std::atomic<bool>& ready) {
    g_screen = Screen::Main;
    render_main_screen(ready);
    exporter::show_menu_window();
}

#endif

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

    exporter::set_menu_key_handler([&ready](wchar_t key) { handle_menu_key(key, ready); });

    std::vector<exporter::TrayMenuItem> items = {
        {L"Статус",
         [&ready] {
             exporter::show_message_box(
                 L"Статус", ready ? L"Авторизован, работает в фоне." : L"Идёт настройка/авторизация...");
         }},
        {L"Показать конфиг", [] { exporter::show_message_box(L"config.json", exporter::utf8_to_wide(read_config_text())); }},
        {L"Открыть меню", [&ready] { open_menu(ready); }},
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
