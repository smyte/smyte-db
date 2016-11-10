#include "infra/kafka/EventCallback.h"

#include "glog/logging.h"
#include "librdkafka/rdkafkacpp.h"

namespace infra {
namespace kafka {

void EventCallback::event_cb(RdKafka::Event& event) {
  switch (event.type()) {
  case RdKafka::Event::EVENT_ERROR:
    processErrorEvent(event);
    break;
  case RdKafka::Event::EVENT_STATS:
    processStatsEvent(event);
    break;
  case RdKafka::Event::EVENT_LOG:
    processLogEvent(event);
    break;
  case RdKafka::Event::EVENT_THROTTLE:
    processThrottleEvent(event);
    break;
  default:
    LOG(ERROR) << "Unknown kafka event type: " << event.type();
    break;
  }
}

}  // namespace kafka
}  // namespace infra
