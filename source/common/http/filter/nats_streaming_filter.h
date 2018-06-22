#pragma once

#include "envoy/nats/streaming/client.h"

#include "common/http/filter/nats_streaming_filter_config.h"
#include "common/http/filter/subject_retriever.h"
#include "common/http/functional_stream_decoder_base.h"

#include "nats_streaming_filter.pb.h"

namespace Envoy {
namespace Http {

using Envoy::Upstream::ClusterManager;

class NatsStreamingFilter : public StreamDecoderFilter,
                            public FunctionalFilter,
                            public Nats::Streaming::PublishCallbacks {
public:
  NatsStreamingFilter(NatsStreamingFilterConfigSharedPtr config,
                      SubjectRetrieverSharedPtr retreiver,
                      Nats::Streaming::ClientPtr nats_streaming_client);
  ~NatsStreamingFilter();

  // Http::StreamFilterBase
  void onDestroy() override;

  // Http::StreamDecoderFilter
  FilterHeadersStatus decodeHeaders(HeaderMap &, bool) override;
  FilterDataStatus decodeData(Buffer::Instance &, bool) override;
  FilterTrailersStatus decodeTrailers(HeaderMap &) override;

  void setDecoderFilterCallbacks(
      StreamDecoderFilterCallbacks &decoder_callbacks) override {
    decoder_callbacks_ = &decoder_callbacks;
    auto decoder_limit = decoder_callbacks.decoderBufferLimit();
    if (decoder_limit > 0) {
      decoder_buffer_limit_ = decoder_limit;
    }
  }

  bool retrieveFunction(const MetadataAccessor &meta_accessor) override;

  // Nats::Streaming::PublishCallbacks
  virtual void onResponse() override;
  virtual void onFailure() override;
  virtual void onTimeout() override;

private:
  void retrieveSubject(const MetadataAccessor &meta_accessor);

  inline bool isActive() { return optional_subject_.has_value(); }

  void relayToNatsStreaming();

  inline void onCompletion(Code response_code, const std::string &body_text);

  inline void onCompletion(Code response_code, const std::string &body_text,
                           RequestInfo::ResponseFlag response_flag);

  const NatsStreamingFilterConfigSharedPtr config_;
  SubjectRetrieverSharedPtr subject_retriever_;
  Nats::Streaming::ClientPtr nats_streaming_client_;
  absl::optional<Subject> optional_subject_;
  StreamDecoderFilterCallbacks *decoder_callbacks_{};
  absl::optional<uint32_t> decoder_buffer_limit_{};
  Buffer::OwnedImpl body_{};
  Nats::Streaming::PublishRequestPtr in_flight_request_{};
};

} // namespace Http
} // namespace Envoy
