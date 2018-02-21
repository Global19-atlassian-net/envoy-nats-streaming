#pragma once

#include <string>

#include "common/singleton/const_singleton.h"

namespace Envoy {
namespace Config {

// TODO(talnordan): TODO: Merge with
// envoy/source/common/config/well_known_names.h.

/**
 * Well-known http filter names.
 */
class SoloHttpFilterNameValues {
public:
  // NATS Streaming filter
  const std::string NATS_STREAMING = "io.solo.nats_streaming";
};

typedef ConstSingleton<SoloHttpFilterNameValues> SoloHttpFilterNames;

/**
 * Well-known metadata filter namespaces.
 */
class SoloMetadataFilterValues {
public:
  // Filter namespace for NATS Streaming Filter.
  const std::string NATS_STREAMING = "io.solo.nats_streaming";
};

typedef ConstSingleton<SoloMetadataFilterValues> SoloMetadataFilters;

/**
 * Keys for MetadataFilterConstants::NATS_STREAMING metadata.
 */
class MetadataNatsStreamingKeyValues {
public:
  // Key in the NATS Streaming Filter namespace for discover prefix value.
  const std::string DISCOVER_PREFIX = "discover_prefix";

  // Key in the NATS Streaming Filter namespace for Cluster ID value.
  const std::string CLUSTER_ID = "cluster_id";
};

typedef ConstSingleton<MetadataNatsStreamingKeyValues>
    MetadataNatsStreamingKeys;

} // namespace Config
} // namespace Envoy
