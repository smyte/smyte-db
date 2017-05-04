#ifndef PIPELINE_EMBEDDEDHTTPSERVER_H_
#define PIPELINE_EMBEDDEDHTTPSERVER_H_

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "civetweb/CivetServer.h"
#include "folly/Conv.h"
#include "glog/logging.h"

namespace pipeline {

// An embedded http server for serving metrics and health check queries. It's not suitable as a standalone server
class EmbeddedHttpServer {
 public:
  using Handler = std::function<bool(std::string*)>;
  class RootHandler : public CivetHandler {
   public:
    explicit RootHandler(const std::unordered_map<std::string, Handler>& _handlerTable)
        : handlerTable(_handlerTable) {}

    bool handleGet(CivetServer* server, struct mg_connection* conn) override;

   private:
    const std::unordered_map<std::string, Handler>& handlerTable;
  };

  explicit EmbeddedHttpServer(int port)
      : port_(port),
        handlerTable_(),
        rootHandler_(nullptr),
        server_(new CivetServer({"listening_ports", folly::to<std::string>(port_), "num_threads",
                                 folly::to<std::string>(kDefaultThreadPoolSize)})),
        run_(true) {}

  // Register a handler for a path. Return whether the registration succeeded.
  // Note that registering after the server started has no effect.
  bool registerHandler(const std::string& path, Handler handler) {
    return handlerTable_.insert(std::make_pair(path, handler)).second;
  }

  std::shared_ptr<CivetServer> getBaseServer() {
    return server_;
  }

  // Start the http server in its own thread. This method returns after the server thread is created.
  void start() {
    CHECK(!rootHandler_);
    rootHandler_.reset(new RootHandler(handlerTable_));
    server_->addHandler("/", rootHandler_.get());
  }

  // Block until the server exists.
  void waitForStop(void) {
    while (run_) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  // Stop the server.
  void stop(void) {
    run_ = false;
    if (server_) {
      server_->close();
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
  static constexpr int kDefaultThreadPoolSize = 16;

  const int port_;
  std::unordered_map<std::string, Handler> handlerTable_;
  std::unique_ptr<RootHandler> rootHandler_;
  std::shared_ptr<CivetServer> server_;
  bool run_;
};

}  // namespace pipeline

#endif  // PIPELINE_EMBEDDEDHTTPSERVER_H_
