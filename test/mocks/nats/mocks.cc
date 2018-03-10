#include "mocks.h"

using testing::Invoke;
using testing::_;

namespace Envoy {
namespace Nats {

bool operator==(const Message &lhs, const Message &rhs) {
  return lhs.asString() == rhs.asString();
}

namespace ConnPool {

MockInstance::MockInstance() {}
MockInstance::~MockInstance() {}

MockManager::MockManager() {}
MockManager::~MockManager() {}

} // namespace ConnPool

} // namespace Nats
} // namespace Envoy
