#include <iostream>

#include "envoy/http/metadata_accessor.h"

#include "common/config/nats_streaming_well_known_names.h"
#include "common/http/filter/metadata_subject_retriever.h"
#include "common/protobuf/utility.h"

#include "test/test_common/utility.h"

#include "fmt/format.h"

namespace Envoy {

using Http::MetadataSubjectRetriever;
using Http::Subject;

namespace {

const std::string empty_json = R"EOF(
    {
    }
    )EOF";

// TODO(talnordan): Move this to `mocks/http/filter`.
class TesterMetadataAccessor : public Http::MetadataAccessor {
public:
  virtual Optional<const std::string *> getFunctionName() const {
    if (!function_name_.empty()) {
      return &function_name_;
    }
    return {};
  }

  virtual Optional<const ProtobufWkt::Struct *> getFunctionSpec() const {
    if (function_spec_ != nullptr) {
      return function_spec_;
    }
    return {};
  }

  virtual Optional<const ProtobufWkt::Struct *> getClusterMetadata() const {
    if (cluster_metadata_ != nullptr) {
      return cluster_metadata_;
    }
    return {};
  }

  virtual Optional<const ProtobufWkt::Struct *> getRouteMetadata() const {
    if (route_metadata_ != nullptr) {
      return route_metadata_;
    }
    return {};
  }

  std::string function_name_;
  const ProtobufWkt::Struct *function_spec_;
  const ProtobufWkt::Struct *cluster_metadata_;
  const ProtobufWkt::Struct *route_metadata_;
};

Protobuf::Struct getMetadata(const std::string &json) {
  Protobuf::Struct metadata;
  MessageUtil::loadFromJson(json, metadata);

  return metadata;
}

TesterMetadataAccessor
getMetadataAccessor(const std::string &function_name,
                    const Protobuf::Struct &func_metadata,
                    const Protobuf::Struct &cluster_metadata,
                    const Protobuf::Struct &route_metadata) {
  TesterMetadataAccessor testaccessor;
  testaccessor.function_name_ = function_name;
  testaccessor.function_spec_ = &func_metadata;
  testaccessor.cluster_metadata_ = &cluster_metadata;
  testaccessor.route_metadata_ = &route_metadata;

  return testaccessor;
}

TesterMetadataAccessor getMetadataAccessorFromJson(
    const std::string &function_name, const std::string &func_json,
    const std::string &cluster_json, const std::string &route_json) {
  auto func_metadata = getMetadata(func_json);
  auto cluster_metadata = getMetadata(cluster_json);
  auto route_metadata = getMetadata(route_json);

  return getMetadataAccessor(function_name, func_metadata, cluster_metadata,
                             route_metadata);
}

} // namespace

TEST(MetadataSubjectRetrieverTest, EmptyJsonAndNoFunctions) {
  const std::string function_name = "";
  const std::string &func_json = empty_json;
  const std::string &cluster_json = empty_json;
  const std::string &route_json = empty_json;

  auto metadata_accessor =
      getMetadataAccessorFromJson("", func_json, cluster_json, route_json);

  MetadataSubjectRetriever subjectRetriever;
  auto subject = subjectRetriever.getSubject(metadata_accessor);

  EXPECT_FALSE(subject.valid());
}

TEST(MetadataSubjectRetrieverTest, ConfiguredSubject) {
  const std::string configuredSubject = "Subject1";

  const std::string func_json = empty_json;
  const std::string cluster_json = empty_json;
  const std::string route_json = empty_json;

  auto metadata_accessor = getMetadataAccessorFromJson(
      configuredSubject, func_json, cluster_json, route_json);

  MetadataSubjectRetriever subjectRetriever;
  auto actualSubject = subjectRetriever.getSubject(metadata_accessor);

  EXPECT_TRUE(actualSubject.valid());
  EXPECT_EQ(*actualSubject.value(), configuredSubject);
}

} // namespace Envoy
