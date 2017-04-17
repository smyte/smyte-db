#include "pipeline/EmbeddedHttpServer.h"

namespace pipeline {

void EmbeddedHttpServer::RootHandler::operator()(const Server::request& request, Server::connection_ptr conn) {
  static Server::response_header headers[] = {{"Connection", "close"}, {"Content-Type", "text/plain"}};
  // NOTE: request.destination may include both path and query. But we assume clients don't provide query for now
  auto it = handlerTable.find(request.destination);
  if (it == handlerTable.end()) {
    conn->set_status(Server::connection::bad_request);
    conn->set_headers(boost::make_iterator_range(headers, headers + 2));
    return;
  }

  std::string response;
  if (it->second(request.destination, &response)) {
    conn->set_status(Server::connection::ok);
  } else {
    conn->set_status(Server::connection::internal_server_error);
  }
  conn->set_headers(boost::make_iterator_range(headers, headers + 2));
  conn->write(response);
}

constexpr int EmbeddedHttpServer::kDefaultThreadPoolSize;

}  // namespace pipeline
