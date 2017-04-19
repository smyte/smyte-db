#ifndef PIPELINE_EMBEDDEDHTTPSERVER_H_
#define PIPELINE_EMBEDDEDHTTPSERVER_H_

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "cpp-netlib/boost/network/protocol/http/server.hpp"
#include "cpp-netlib/boost/network/utils/thread_pool.hpp"
#include "folly/Conv.h"
#include "glog/logging.h"

namespace pipeline {

// An embedded http server for serving metrics and health check queries. It's not suitable as a standalone server
class EmbeddedHttpServer {
 public:
  struct RootHandler;
  using Server = boost::network::http::server<RootHandler>;
  using Handler = std::function<bool(const std::string&, std::string*)>;
  struct RootHandler {
    explicit RootHandler(const std::unordered_map<std::string, Handler>& _handlerTable)
        : handlerTable(_handlerTable) {}

    void operator()(const Server::request& request, Server::connection_ptr conn);

    const std::unordered_map<std::string, Handler>& handlerTable;
  };

  explicit EmbeddedHttpServer(int port) : port_(port), handlerTable_(), rootHandler_(nullptr), server_(nullptr) {}

  // Register a handler for a path. Return whether the registration succeeded.
  // Note that registering after the server started has no effect.
  bool registerHandler(const std::string& path, Handler handler) {
    return handlerTable_.insert(std::make_pair(path, handler)).second;
  }

  // Start the http server in its own thread. This method returns after the server thread is created.
  void start() {
    CHECK(!handlerTable_.empty());
    CHECK(!rootHandler_ && !server_ && !serverThread_);

    rootHandler_.reset(new RootHandler(handlerTable_));
    server_.reset(
        new Server(Server::options(*rootHandler_)
                       .reuse_address(true)
                       .address("::")
                       .port(folly::to<std::string>(port_))
                       .thread_pool(std::make_shared<boost::network::utils::thread_pool>(kDefaultThreadPoolSize))));

    // Start http server in its own thread. The server has its own internal thread_pool for processing requests
    serverThread_.reset(new std::thread([this]() {
      try {
        server_->run();
      } catch (std::exception &e) {
        LOG(ERROR) << "EmbeddedHttpServer failed: " << e.what();
      }
    }));
  }

  // Block until the server exists.
  void waitForStop(void) {
    CHECK(serverThread_ != nullptr) << "Server thread has not been created";

    if (serverThread_->joinable()) {
      serverThread_->join();
    }
  }

  // Stop the server.
  void stop(void) {
    if (server_) {
      server_->stop();
    }
  }

  // Destroy resource for this server.
  void destroy(void) {
    stop();
    waitForStop();
    server_.reset();
    LOG(INFO) << "Embedded http server has stopped gracefully";
  }

 private:
  static constexpr int kDefaultThreadPoolSize = 4;

  const int port_;
  std::unordered_map<std::string, Handler> handlerTable_;
  std::unique_ptr<RootHandler> rootHandler_;
  std::unique_ptr<Server> server_;
  std::unique_ptr<std::thread> serverThread_;
};

}  // namespace pipeline

#endif  // PIPELINE_EMBEDDEDHTTPSERVER_H_
