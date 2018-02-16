#include "mocks.h"

#include "common/common/macros.h"

using testing::Invoke;
using testing::_;

namespace Envoy {
namespace Nats {

bool operator==(const Message &lhs, const Message &rhs) {
  return lhs.asString() == rhs.asString();
}

namespace Publisher {

MockPublishCallbacks::MockPublishCallbacks() {}
MockPublishCallbacks::~MockPublishCallbacks() {}

MockInstance::MockInstance() {
  ON_CALL(*this, makeRequest(_, _, _, _))
      .WillByDefault(Invoke(
          [this](const std::string &cluster_name, const std::string &subject,
                 const Buffer::Instance *payload,
                 PublishCallbacks &callbacks) -> PublishRequestPtr {
            UNREFERENCED_PARAMETER(cluster_name);
            UNREFERENCED_PARAMETER(subject);
            UNREFERENCED_PARAMETER(payload);

            last_payload_ = payload;

            callbacks.onResponse();
            return nullptr;
          }));
}

MockInstance::~MockInstance() {}

} // namespace Publisher

namespace ConnPool {

MockManager::MockManager() {}
MockManager::~MockManager() {}

} // namespace ConnPool

} // namespace Nats
} // namespace Envoy
