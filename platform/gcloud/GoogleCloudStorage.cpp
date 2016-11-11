#include "platform/gcloud/GoogleCloudStorage.h"

#include <memory>
#include <string>

#include "folly/FileUtil.h"
#include "glog/logging.h"
#include "googleapis/client/data/data_reader.h"
#include "googleapis/client/transport/http_request.h"
#include "googleapis/client/transport/http_response.h"

namespace platform {
namespace gcloud {

using google_storage_api::Object;
using google_storage_api::ObjectsResource_GetMethod;
using googleapis::client::DataReader;
using googleapis::client::HttpRequest;
using googleapis::util::Status;

googleapis::util::Status GoogleCloudStorage::getObject(const std::string& bucketName, const std::string& objectName,
                                                       Object* object) {
  authenticate();

  const auto& objectsResource = storageService_->get_objects();
  auto get =
      std::unique_ptr<ObjectsResource_GetMethod>(objectsResource.NewGetMethod(&credential_, bucketName, objectName));
  return get->ExecuteAndParseResponse(object);
}

Status GoogleCloudStorage::downloadObject(const std::string& bucketName, const std::string& objectName,
                                          const std::string& downloadPath, google_storage_api::Object* object) {
  authenticate();

  // Get object metadata
  Status status = getObject(bucketName, objectName, object);
  if (!status.ok()) return status;

  // Download object data
  auto request = std::unique_ptr<HttpRequest>(httpTransport_->NewHttpRequest(HttpRequest::GET));
  request->set_url(object->get_media_link().data());
  status = credential_.AuthorizeRequest(request.get());
  if (!status.ok()) return status;
  status = request->Execute();
  if (!status.ok()) return status;
  DataReader* dataReader = request->response()->body_reader();
  // TODO(yunjing): we read everything into memory then save it to file. Should stream it instead
  // Panic on failure instead of returning status because failed to write to a local file system is hardly recoverable
  CHECK(folly::writeFile(dataReader->RemainderToString(), downloadPath.data()))
      << "Fail to write object " << bucketName << "/" << objectName << " to " << downloadPath;
  if (dataReader->done()) {
    return Status();
  } else {
    return dataReader->status();
  }
}

void GoogleCloudStorage::authenticate(void) {
  if (!credential_.access_token().empty() &&
      nowSec() + kCredentialExpirationMarginSec < credential_.expiration_timestamp_secs()) {
    // Credential is still valid, no need to refresh
    return;
  }
  // Need to refresh access token
  // TODO(yunjing): implement generic OAuth2
  updateCredentialJsonFromGce();
}

void GoogleCloudStorage::updateCredentialJsonFromGce(void) {
  LOG(INFO) << "Refreshing credential from GCE";
  auto request = std::unique_ptr<HttpRequest>(httpTransport_->NewHttpRequest(HttpRequest::GET));
  request->set_url(kGceCredentialUrl);
  request->AddHeader("Metadata-Flavor", "Google");
  Status status = request->Execute();
  CHECK(status.ok()) << status.ToString();
  status = credential_.Update(request->response()->body_reader());
  CHECK(status.ok()) << status.ToString();
}

constexpr char GoogleCloudStorage::kGceCredentialUrl[];

}  // namespace gcloud
}  // namespace platform
