#pragma once

#include <map>
#include <string>

#include "envoy/common/optional.h"
#include "envoy/event/timer.h"
#include "envoy/nats/streaming/client.h"
#include "envoy/nats/streaming/inbox_handler.h"

namespace Envoy {
namespace Nats {
namespace Streaming {

// TODO(talnordan): Consider moving to `include/envoy`.
struct PubRequest {
  PublishCallbacks *callbacks;
  Event::TimerPtr timeout_timer;
};

class PubRequestHandler {
public:
  static void onMessage(const Optional<std::string> &reply_to,
                        const std::string &payload,
                        InboxCallbacks &inbox_callbacks,
                        PublishCallbacks &publish_callbacks);

  static void onMessage(const std::string &inbox,
                        const Optional<std::string> &reply_to,
                        const std::string &payload,
                        InboxCallbacks &inbox_callbacks,
                        std::map<std::string, PubRequest> &request_per_inbox);

  static void onTimeout(const std::string &inbox,
                        std::map<std::string, PubRequest> &request_per_inbox);

private:
  static inline void
  eraseRequest(std::map<std::string, PubRequest> &request_per_inbox,
               std::map<std::string, PubRequest>::iterator position);
};

} // namespace Streaming
} // namespace Nats
} // namespace Envoy