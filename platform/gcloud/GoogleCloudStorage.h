#ifndef PLATFORM_GCLOUD_GOOGLECLOUDSTORAGE_H_
#define PLATFORM_GCLOUD_GOOGLECLOUDSTORAGE_H_

#include <chrono>
#include <memory>
#include <string>

#include "glog/logging.h"
#include "google/storage_api/storage_service.h"
#include "googleapis/client/auth/oauth2_authorization.h"
#include "googleapis/client/transport/curl_http_transport.h"
#include "googleapis/client/transport/http_transport.h"
#include "googleapis/util/status.h"

namespace platform {
namespace gcloud {

// Implement ObjectStore using Google Cloud Storage service.
class GoogleCloudStorage {
 public:
  explicit GoogleCloudStorage(const std::string& caCertsPath = "")
      : httpConfig_(new googleapis::client::HttpTransportLayerConfig()),
        httpTransport_(nullptr),
        storageService_(nullptr),
        credential_() {
    httpConfig_->ResetDefaultTransportFactory(new googleapis::client::CurlHttpTransportFactory(httpConfig_.get()));
    if (caCertsPath.empty()) {
      httpConfig_->mutable_default_transport_options()->set_cacerts_path(
          googleapis::client::HttpTransportOptions::kDisableSslVerification);
    } else {
      httpConfig_->mutable_default_transport_options()->set_cacerts_path(caCertsPath);
    }
    httpTransport_.reset(httpConfig_->NewDefaultTransportOrDie());
    storageService_.reset(new google_storage_api::StorageService(httpConfig_->NewDefaultTransportOrDie()));
  }


  // Get object metadata from GCS
  googleapis::util::Status getObject(const std::string& bucketName, const std::string& objectName,
                                     google_storage_api::Object* object);
  // Download an object data from GCS to the given destination
  googleapis::util::Status downloadObject(const std::string& bucketName, const std::string& objectName,
                                          const std::string& downloadPath, google_storage_api::Object* object);

 private:
  static constexpr int64_t kCredentialExpirationMarginSec = 30;
  static constexpr char kGceCredentialUrl[] =
      "http://metadata.google.internal/computeMetadata/v1/instance/service-accounts/default/token";

  static int64_t nowSec() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  // Make sure credential_ is still valid by authenticating with Google Cloud when needed
  // TODO(yunjing): Build a more general authentication mechanism to support other Google Cloud services
  googleapis::util::Status authenticate(void);
  // Update credential by talking to GCE metadata server
  googleapis::util::Status updateCredentialJsonFromGce(void);

  std::unique_ptr<googleapis::client::HttpTransportLayerConfig> httpConfig_;
  std::unique_ptr<googleapis::client::HttpTransport> httpTransport_;
  std::unique_ptr<google_storage_api::StorageService> storageService_;
  googleapis::client::OAuth2Credential credential_;
};

}  // namespace gcloud
}  // namespace platform

#endif  // PLATFORM_GCLOUD_GOOGLECLOUDSTORAGE_H_
