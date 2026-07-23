#pragma once

#include "core/tdlib_client.h"

#include <cstdint>
#include <string>

namespace exporter {
    struct AuthConfig {
        std::int32_t api_id = 0;
        std::string api_hash;
        std::string session_path = "./session";
    };

    // Drives TDLib's authorizationState machine to completion, prompting on
    // stdin/stdout for phone number / code / 2FA password when TDLib asks for
    // them. Registers itself as the client's update handler for the duration of
    // its lifetime.
    class Auth {
        public:
            Auth(TdlibClient& client, AuthConfig config);

            // Pumps client.poll() until authorizationStateReady or
            // authorizationStateClosed is reached.
            void wait_until_ready();

            bool is_ready() const {
                return ready_;
            }

        private:
            TdlibClient& client_;
            AuthConfig config_;
            bool ready_ = false;
            bool closed_ = false;

            void handle_update(td_api::object_ptr<td_api::Object> update);
            void handle_authorization_state(td_api::object_ptr<td_api::AuthorizationState> state);
            void send_auth_query(td_api::object_ptr<td_api::Function> function);
    };

}  // namespace exporter
