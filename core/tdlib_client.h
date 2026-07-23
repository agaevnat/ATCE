#pragma once

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>

namespace exporter {

    namespace td_api = td::td_api;

    // Thin wrapper around td::ClientManager: owns the client, hands out query
    // ids, and routes responses back to the caller that sent the query (or, for
    // request_id == 0, to the registered push-update handler). Carries no
    // Telegram-domain logic of its own — that belongs to callers like Auth.
    class TdlibClient {
        public:
            using Object = td_api::object_ptr<td_api::Object>;
            using ResponseHandler = std::function<void(Object)>;
            using UpdateHandler = std::function<void(Object)>;

            TdlibClient();

            void set_update_handler(UpdateHandler handler);

            void send(td_api::object_ptr<td_api::Function> function, ResponseHandler handler = {});

            // Blocks up to timeout_seconds for the next response/update and
            // dispatches it. Returns immediately if one is already pending.
            void poll(double timeout_seconds);

        private:
            std::unique_ptr<td::ClientManager> client_manager_;
            std::int32_t client_id_ = 0;
            std::uint64_t next_query_id_ = 0;
            std::map<std::uint64_t, ResponseHandler> handlers_;
            UpdateHandler update_handler_;

            void dispatch(td::ClientManager::Response response);
    };

}  // namespace exporter
