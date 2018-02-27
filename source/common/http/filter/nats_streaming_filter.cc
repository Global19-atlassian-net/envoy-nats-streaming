#include "common/http/filter/nats_streaming_filter.h"

#include <algorithm>
#include <list>
#include <string>
#include <vector>

#include "envoy/http/header_map.h"

#include "common/common/empty_string.h"
#include "common/common/macros.h"
#include "common/common/utility.h"
#include "common/http/filter_utility.h"
#include "common/http/solo_filter_utility.h"
#include "common/http/utility.h"

#include "server/config/network/http_connection_manager.h"

namespace Envoy {
namespace Http {

NatsStreamingFilter::NatsStreamingFilter(
    Server::Configuration::FactoryContext &ctx, const std::string &name,
    NatsStreamingFilterConfigSharedPtr config,
    SubjectRetrieverSharedPtr retreiver, Nats::Publisher::InstancePtr publisher)
    : FunctionalFilterBase(ctx, name), config_(config),
      subject_retriever_(retreiver), publisher_(publisher) {}

NatsStreamingFilter::~NatsStreamingFilter() {}

Envoy::Http::FilterHeadersStatus
NatsStreamingFilter::functionDecodeHeaders(Envoy::Http::HeaderMap &headers,
                                           bool end_stream) {
  UNREFERENCED_PARAMETER(headers);
  RELEASE_ASSERT(isActive());

  if (end_stream) {
    relayToNatsStreaming();
  }

  return Envoy::Http::FilterHeadersStatus::StopIteration;
}

Envoy::Http::FilterDataStatus
NatsStreamingFilter::functionDecodeData(Envoy::Buffer::Instance &data,
                                        bool end_stream) {
  UNREFERENCED_PARAMETER(data);
  RELEASE_ASSERT(isActive());

  if (end_stream) {
    relayToNatsStreaming();

    // TODO(talnordan): We need to make sure that life time of the buffer makes
    // sense.
    return Envoy::Http::FilterDataStatus::StopIterationAndBuffer;
  }

  return Envoy::Http::FilterDataStatus::StopIterationAndBuffer;
}

Envoy::Http::FilterTrailersStatus
NatsStreamingFilter::functionDecodeTrailers(Envoy::Http::HeaderMap &) {
  RELEASE_ASSERT(isActive());

  relayToNatsStreaming();
  return Envoy::Http::FilterTrailersStatus::StopIteration;
}

bool NatsStreamingFilter::retrieveFunction(
    const MetadataAccessor &meta_accessor) {
  retrieveSubject(meta_accessor);
  return isActive();
}

void NatsStreamingFilter::onResponse() {
  Http::Utility::sendLocalReply(*decoder_callbacks_, stream_destroyed_,
                                Http::Code::OK, "");
}

void NatsStreamingFilter::onFailure() {
  decoder_callbacks_->requestInfo().setResponseFlag(
      RequestInfo::ResponseFlag::FaultInjected);
  Http::Utility::sendLocalReply(*decoder_callbacks_, stream_destroyed_,
                                Http::Code::InternalServerError,
                                "nats streaming filter abort");
}

void NatsStreamingFilter::retrieveSubject(
    const MetadataAccessor &meta_accessor) {
  optional_subject_ = subject_retriever_->getSubject(meta_accessor);
}

void NatsStreamingFilter::relayToNatsStreaming() {
  RELEASE_ASSERT(optional_subject_.valid());
  RELEASE_ASSERT(!optional_subject_.value()->empty());

  const std::string *cluster_name =
      SoloFilterUtility::resolveClusterName(decoder_callbacks_);
  if (!cluster_name) {
    // TODO(talnordan): Consider changing the return type to `bool` and
    // returning `false`.
    return;
  }

  const std::string &subject = *optional_subject_.value();
  const Buffer::Instance *payload = decoder_callbacks_->decodingBuffer();

  // TODO(talnordan): Keep the return value of `makeRequest()`.
  // TODO(talnordan): Who is responsible for freeing `payload`'s memory?
  publisher_->makeRequest(*cluster_name, subject, payload, *this);
}

} // namespace Http
} // namespace Envoy
