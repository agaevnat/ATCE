#include "core/auth.h"
#include "core/console.h"
#include "core/td_overload.h"

#include <iostream>

namespace exporter {

    static std::string prompt_line(const std::string& prompt) {
        ensure_console_visible();
        std::cout << prompt << std::flush;
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    Auth::Auth(TdlibClient& client, AuthConfig config) : client_(client), config_(std::move(config)) {
        client_.set_update_handler([this](td_api::object_ptr<td_api::Object> update) { handle_update(std::move(update)); });
    }

    void Auth::wait_until_ready() {
        while (!ready_ && !closed_) {
            client_.poll(10.0);
        }
    }

    void Auth::handle_update(td_api::object_ptr<td_api::Object> update) {
        if (update->get_id() != td_api::updateAuthorizationState::ID) {
            return;
        }
        auto& update_state = static_cast<td_api::updateAuthorizationState&>(*update);
        handle_authorization_state(std::move(update_state.authorization_state_));
    }

    void Auth::send_auth_query(td_api::object_ptr<td_api::Function> function) {
        client_.send(std::move(function), [](td_api::object_ptr<td_api::Object> object) {
            if (object->get_id() == td_api::error::ID) {
                auto& error = static_cast<td_api::error&>(*object);
                std::cout << "Ошибка авторизации (" << error.code_ << "): " << error.message_ << std::endl;
            }
        });
    }

    void Auth::handle_authorization_state(td_api::object_ptr<td_api::AuthorizationState> state) {
        td_api::downcast_call(
            *state,
            overloaded(
                [this](td_api::authorizationStateWaitTdlibParameters&) {
                    // setTdlibParameters used to take a nested tdlibParameters object;
                    // current TDLib flattens the fields directly onto the function call,
                    // and the separate encryption-key-check step (authorizationStateWaitEncryptionKey)
                    // was removed along with it.
                    send_auth_query(td_api::make_object<td_api::setTdlibParameters>(
                        /* use_test_dc */ false,
                        /* database_directory */ config_.session_path,
                        /* files_directory */ std::string{},
                        /* database_encryption_key */ std::string{},
                        /* use_file_database */ true,
                        /* use_chat_info_database */ true,
                        /* use_message_database */ true,
                        /* use_secret_chats */ true,
                        /* api_id */ config_.api_id,
                        /* api_hash */ config_.api_hash,
                        /* system_language_code */ "ru",
                        /* device_model */ "TelegramChatExporter",
                        /* system_version */ std::string{},
                        /* application_version */ "0.1"));
                },
                [this](td_api::authorizationStateWaitPremiumPurchase&) {
                    std::cout << "Для входа требуется покупка Telegram Premium — такой сценарий не поддерживается." << std::endl;
                    closed_ = true;
                },
                [this](td_api::authorizationStateWaitEmailAddress&) {
                    std::cout << "Для входа требуется email — такой сценарий не поддерживается, только номер телефона." << std::endl;
                    closed_ = true;
                },
                [this](td_api::authorizationStateWaitEmailCode&) {
                    std::cout << "Для входа требуется код с email — такой сценарий не поддерживается." << std::endl;
                    closed_ = true;
                },
                [this](td_api::authorizationStateWaitPhoneNumber&) {
                    auto phone_number = prompt_line("Номер телефона (в международном формате, напр. +79991234567): ");
                    send_auth_query(td_api::make_object<td_api::setAuthenticationPhoneNumber>(phone_number, nullptr));
                },
                [this](td_api::authorizationStateWaitCode&) {
                    auto code = prompt_line("Код из Telegram: ");
                    send_auth_query(td_api::make_object<td_api::checkAuthenticationCode>(code));
                },
                [this](td_api::authorizationStateWaitPassword&) {
                    auto password = prompt_line("Пароль двухфакторной аутентификации: ");
                    send_auth_query(td_api::make_object<td_api::checkAuthenticationPassword>(password));
                },
                [this](td_api::authorizationStateWaitRegistration&) {
                    auto first_name = prompt_line("Аккаунт не зарегистрирован. Имя: ");
                    auto last_name = prompt_line("Фамилия (можно оставить пустой): ");
                    send_auth_query(td_api::make_object<td_api::registerUser>(first_name, last_name, /* disable_notification */ false));
                },
                [](td_api::authorizationStateWaitOtherDeviceConfirmation& wait_state) {
                    std::cout << "Подтвердите вход на другом устройстве по ссылке: " << wait_state.link_ << std::endl;
                },
                [this](td_api::authorizationStateReady&) {
                    ready_ = true;
                    std::cout << "Авторизация прошла успешно." << std::endl;
                },
                [](td_api::authorizationStateLoggingOut&) { std::cout << "Выполняется выход из аккаунта..." << std::endl; },
                [](td_api::authorizationStateClosing&) { std::cout << "Закрытие соединения..." << std::endl; },
                [this](td_api::authorizationStateClosed&) {
                    closed_ = true;
                    std::cout << "Соединение с TDLib закрыто." << std::endl;
                }));
    }

}  // namespace exporter
