#include "core/tdlib_client.h"

namespace exporter {
    TdlibClient::TdlibClient() {
        td::ClientManager::execute(td_api::make_object<td_api::setLogVerbosityLevel>(1));
        client_manager_ = std::make_unique<td::ClientManager>();
        client_id_ = client_manager_->create_client_id();
        // Any query wakes the client up and starts the authorizationState pushes.
        send(td_api::make_object<td_api::getOption>("version"));
    }

    void TdlibClient::set_update_handler(UpdateHandler handler) {
        update_handler_ = std::move(handler);
    }

    void TdlibClient::send(td_api::object_ptr<td_api::Function> function, ResponseHandler handler) {
        auto query_id = ++next_query_id_;
        if (handler) {
            handlers_.emplace(query_id, std::move(handler));
        }
        client_manager_->send(client_id_, query_id, std::move(function));
    }

    void TdlibClient::poll(double timeout_seconds) {
        dispatch(client_manager_->receive(timeout_seconds));
    }

    void TdlibClient::dispatch(td::ClientManager::Response response) {
        if (!response.object) {
            return;
        }
        if (response.request_id == 0) {
            if (update_handler_) {
                update_handler_(std::move(response.object));
            }
            return;
        }
        auto it = handlers_.find(response.request_id);
        if (it != handlers_.end()) {
            it->second(std::move(response.object));
            handlers_.erase(it);
        }
    }

}  // namespace exporter
