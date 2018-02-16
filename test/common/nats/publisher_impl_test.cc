#include "common/nats/publisher_impl.h"

#include "test/mocks/nats/mocks.h"
#include "test/mocks/tcp/mocks.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {
namespace Nats {
namespace Publisher {

class NatsPublisherImplTest : public testing::Test {
public:
  Tcp::ConnPool::ManagerPtr<std::string> conn_pool_manager_ =
      std::make_shared<Tcp::ConnPool::MockManager>();
  InstanceImpl publisher_{
      Tcp::ConnPool::ManagerPtr<std::string>{conn_pool_manager_}};
  MockPublishCallbacks callbacks_;
  PublishRequestPtr handle_;
};

TEST_F(NatsPublisherImplTest, Empty) {
  // TODO(talnordan): This is a dummy test.
}

} // namespace Publisher
} // namespace Nats
} // namespace Envoy
