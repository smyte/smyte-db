#include "pipeline/EmbeddedHttpServer.h"

namespace pipeline {

bool EmbeddedHttpServer::RootHandler::handleGet(CivetServer* server, struct mg_connection* conn) {
  auto reqInfo = mg_get_request_info(conn);
  auto it = handlerTable.find(reqInfo->local_uri);
  if (it == handlerTable.end()) {
    mg_printf(conn, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n");
    return true;
  }

  std::string response;
  if (it->second(&response)) {
    mg_printf(conn, "HTTP/1.1 200 OK\r\n");
  } else {
    mg_printf(conn, "HTTP/1.1 500 Internal Server Error\r\n");
  }
  mg_printf(conn, "Content-Type: text/plain\r\nConnection: close\r\n\r\n");
  mg_printf(conn, response.data(), response.size());
  return true;
}

constexpr int EmbeddedHttpServer::kDefaultThreadPoolSize;

}  // namespace pipeline
