#include "common/http/filter/nats_streaming_filter_config.h"

#include "server/config/http/nats_streaming_filter_config_factory.h"

#include "test/test_common/utility.h"

namespace Envoy {

using Http::NatsStreamingFilterConfig;
using Server::Configuration::NatsStreamingFilterConfigFactory;

namespace {

NatsStreamingFilterConfig
constructNatsStrteamingFilterConfigFromJson(const Json::Object &config) {
  auto proto_config =
      NatsStreamingFilterConfigFactory::translateNatsStreamingFilter(config);
  return NatsStreamingFilterConfig(proto_config);
}

} // namespace

TEST(NatsStreamingFilterConfigTest, Empty) {
  std::string json = R"EOF(
    {
    }
    )EOF";

  Envoy::Json::ObjectSharedPtr json_config =
      Envoy::Json::Factory::loadFromString(json);

  EXPECT_THROW(NatsStreamingFilterConfigFactory::translateNatsStreamingFilter(
                   *json_config),
               Envoy::EnvoyException);
}

TEST(NatsStreamingFilterConfigTest, op_timeout_ms) {
  std::string json = R"EOF(
    {
      "op_timeout_ms" : 17
    }
    )EOF";

  Envoy::Json::ObjectSharedPtr json_config =
      Envoy::Json::Factory::loadFromString(json);
  auto config = constructNatsStrteamingFilterConfigFromJson(*json_config);

  EXPECT_EQ(config.op_timeout(), std::chrono::milliseconds(17));
}

} // namespace Envoy
