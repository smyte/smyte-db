#include "infra/kafka/Producer.h"

#include <memory>
#include <string>

#include "glog/logging.h"
#include "infra/kafka/EventCallback.h"
#include "librdkafka/rdkafkacpp.h"

namespace infra {
namespace kafka {

void Producer::initialize() {
  setConf("metadata.broker.list", brokerList_);
  // disable verbose logging
  setConf("log.connection.close", "false");

  if (useCompression_) {
    // use snappy codec when compression is requested
    setConf("compression.codec", "snappy");
  }

  std::string errstr;

  if (conf_->set("event_cb", static_cast<RdKafka::EventCb*>(this), errstr) != RdKafka::Conf::CONF_OK) {
    LOG(FATAL) << "Setting Kafka event callback failed: " << errstr;
  }

  if (conf_->set("dr_cb", static_cast<RdKafka::DeliveryReportCb*>(this), errstr) != RdKafka::Conf::CONF_OK) {
    LOG(FATAL) << "Setting Kafka delivery report callback failed: " << errstr;
  }

  // by default, require messages to be committed by all in sync replica (ISRs)
  if (topicConf_->set("request.required.acks", "-1", errstr) != RdKafka::Conf::CONF_OK) {
    LOG(FATAL) << "Setting request.required.acks for topic ["  << topicStr_ << "] failed: " << errstr;
  }

  // always return the produced offset to the client
  if (topicConf_->set("produce.offset.report", "true", errstr) != RdKafka::Conf::CONF_OK) {
    LOG(FATAL) << "Setting produce.offset.report for topic ["  << topicStr_ << "] failed: " << errstr;
  }
  if (partitioner_ && topicConf_->set(
      "partitioner_cb", static_cast<RdKafka::PartitionerCb*>(this), errstr) != RdKafka::Conf::CONF_OK) {
    LOG(FATAL) << "Setting partitioner callback for topic ["  << topicStr_ << "] failed: " << errstr;
  }

  producer_.reset(RdKafka::Producer::create(conf_.get(), errstr));
  if (!producer_) {
    LOG(FATAL) << "Failed to create producer: " << errstr;
  }

  topic_.reset(RdKafka::Topic::create(producer_.get(), topicStr_, topicConf_.get(), errstr));
  if (!topic_) {
    LOG(FATAL) << "Failed to create producer topic object for topic [" << topicStr_ << "]: " << errstr;
  }

  RdKafka::Metadata* metadataTmp = nullptr;
  auto errorCode = producer_->metadata(false /* one topic only */, topic_.get(), &metadataTmp, 10000);
  std::unique_ptr<RdKafka::Metadata> metadata(metadataTmp);
  if (errorCode != RdKafka::ERR_NO_ERROR) {
    LOG(FATAL) << "Getting topic metadata failed: " << RdKafka::err2str(errorCode);
  }

  LOG(INFO) << "Kafka producer initialized: " << producer_->name();
}

}  // namespace kafka
}  // namespace infra
