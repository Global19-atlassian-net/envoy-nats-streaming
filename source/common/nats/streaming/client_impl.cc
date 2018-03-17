#include "common/nats/streaming/client_impl.h"

#include "common/common/assert.h"
#include "common/common/macros.h"
#include "common/common/utility.h"
#include "common/nats/message_builder.h"
#include "common/nats/streaming/pub_ack_handler.h"

namespace Envoy {
namespace Nats {
namespace Streaming {

const std::string ClientImpl::INBOX_PREFIX{"_INBOX"};
const std::string ClientImpl::PUB_ACK_PREFIX{"_STAN.acks"};

ClientImpl::ClientImpl(Tcp::ConnPool::InstancePtr<Message> &&conn_pool_,
                       Runtime::RandomGenerator &random)
    : conn_pool_(std::move(conn_pool_)), token_generator_(random),
      heartbeat_inbox_(
          SubjectUtility::randomChild(INBOX_PREFIX, token_generator_)),
      root_inbox_(SubjectUtility::randomChild(INBOX_PREFIX, token_generator_)),
      connect_response_inbox_(
          SubjectUtility::randomChild(root_inbox_, token_generator_)),
      sid_(1) {}

PublishRequestPtr ClientImpl::makeRequest(const std::string &subject,
                                          const std::string &cluster_id,
                                          const std::string &discover_prefix,
                                          Buffer::Instance &payload,
                                          PublishCallbacks &callbacks) {
  cluster_id_.value(cluster_id);
  discover_prefix_.value(discover_prefix);
  outbound_request_.value({subject, drainBufferToString(payload), &callbacks});

  conn_pool_->setPoolCallbacks(*this);

  // Send a NATS CONNECT message.
  sendNatsMessage(MessageBuilder::createConnectMessage());

  // TODO(talnordan)
  return nullptr;
}

void ClientImpl::onResponse(Nats::MessagePtr &&value) {
  ENVOY_LOG(trace, "on response: value is\n[{}]", value->asString());

  // Check whether a payload is expected prior to NATS operation extraction.
  // TODO(talnordan): Eventually, we might want `onResponse()` to be passed a
  // single decoded message consisting of both the `MSG` arguments and the
  // payload.
  if (isWaitingForPayload()) {
    onPayload(std::move(value));
  } else {
    onOperation(std::move(value));
  }
}

void ClientImpl::onClose() {
  // TODO(talnordan)
}

void ClientImpl::onFailure(const std::string &error) {
  // TODO(talnordan): Error handling.
  ENVOY_LOG(error, "on failure: error is\n[{}]", error);
}

void ClientImpl::onConnected(const std::string &pub_prefix) {
  pub_prefix_.value(pub_prefix);

  // TODO(talnordan): Support publishing multiple enqued messages.
  pubPubMsg();
}

void ClientImpl::send(const Message &message) { sendNatsMessage(message); }

void ClientImpl::onOperation(Nats::MessagePtr &&value) {
  // TODO(talnordan): For better performance, a future decoder implementation
  // might use zero allocation byte parsing. In such case, this function would
  // need to switch over an `enum class` representing the message type. See:
  // https://github.com/nats-io/go-nats/blob/master/parser.go
  // https://youtu.be/ylRKac5kSOk?t=10m46s

  auto delimiters = " \t";
  auto keep_empty_string = false;
  auto tokens =
      StringUtil::splitToken(value->asString(), delimiters, keep_empty_string);

  auto &&op = tokens[0];
  if (StringUtil::caseCompare(op, "INFO")) {
    onInfo(std::move(value));
  } else if (StringUtil::caseCompare(op, "MSG")) {
    onMsg(std::move(tokens));
  } else if (StringUtil::caseCompare(op, "PING")) {
    onPing();
  } else {
    // TODO(talnordan): Error handling.
    // TODO(talnordan): Increment error stats.
    throw ProtocolError("invalid message");
  }
}

void ClientImpl::onPayload(Nats::MessagePtr &&value) {
  std::string &subject = getSubjectWaitingForPayload();
  Optional<std::string> &reply_to = getReplyToWaitingForPayload();
  std::string &payload = value->asString();
  if (subject == heartbeat_inbox_) {
    HeartbeatHandler::onMessage(reply_to, payload, *this);
  } else if (subject == connect_response_inbox_) {
    ConnectResponseHandler::onMessage(reply_to, payload, *this);
  } else {
    PubAckHandler::onMessage(reply_to, payload, *this,
                             *callbacks_per_pub_ack_inbox_[subject]);
    callbacks_per_pub_ack_inbox_.erase(subject);
  }

  // Mark that the payload has been received.
  doneWaitingForPayload();
}

void ClientImpl::onInfo(Nats::MessagePtr &&value) {
  // TODO(talnordan): Process `INFO` options.
  UNREFERENCED_PARAMETER(value);

  // TODO(talnordan): The following behavior is part of the PoC implementation.
  subHeartbeatInbox();
  subReplyInbox();
  pubConnectRequest();
}

void ClientImpl::onMsg(std::vector<absl::string_view> &&tokens) {
  auto num_tokens = tokens.size();
  switch (num_tokens) {
  case 4:
    waitForPayload(std::string(tokens[1]), Optional<std::string>{});
    break;
  case 5:
    waitForPayload(std::string(tokens[1]),
                   Optional<std::string>(std::string(tokens[3])));
    break;
  default:
    // TODO(talnordan): Error handling.
    throw ProtocolError("invalid MSG");
  }
}

void ClientImpl::onPing() { pong(); }

void ClientImpl::subInbox(const std::string &subject) {
  sendNatsMessage(MessageBuilder::createSubMessage(subject, sid_));
  ++sid_;
}

void ClientImpl::subHeartbeatInbox() { subInbox(heartbeat_inbox_); }

void ClientImpl::subReplyInbox() {
  std::string root_inbox_child_wildcard{
      SubjectUtility::childWildcard(root_inbox_)};
  subInbox(root_inbox_child_wildcard);
}

void ClientImpl::pubConnectRequest() {
  const std::string subject{
      SubjectUtility::join(discover_prefix_.value(), cluster_id_.value())};

  // TODO(talnordan): Avoid using hard-coded string literals.
  const std::string connect_request_message =
      MessageUtility::createConnectRequestMessage("client1", heartbeat_inbox_);

  pubNatsStreamingMessage(subject, connect_response_inbox_,
                          connect_request_message);
}

void ClientImpl::pubPubMsg() {
  auto &&subject = outbound_request_.value().subject;
  auto &&payload = outbound_request_.value().payload;
  auto &&callbacks = outbound_request_.value().callbacks;
  std::string pub_ack_inbox{
      SubjectUtility::randomChild(PUB_ACK_PREFIX, token_generator_)};

  // TODO(talnordan): `UNSUB` once the response has arrived.
  callbacks_per_pub_ack_inbox_[pub_ack_inbox] = callbacks;
  subInbox(pub_ack_inbox);

  const std::string pub_subject{
      SubjectUtility::join(pub_prefix_.value(), subject)};

  // TODO(talnordan): Avoid using hard-coded string literals.
  const std::string pub_msg_message =
      MessageUtility::createPubMsgMessage("client1", "guid1", subject, payload);

  pubNatsStreamingMessage(pub_subject, pub_ack_inbox, pub_msg_message);
}

void ClientImpl::pong() {
  sendNatsMessage(MessageBuilder::createPongMessage());
}

inline void ClientImpl::sendNatsMessage(const Message &message) {
  // TODO(talnordan): Manage hash key computation.
  const std::string hash_key;

  conn_pool_->makeRequest(hash_key, message);
}

inline void ClientImpl::pubNatsStreamingMessage(const std::string &subject,
                                                const std::string &reply_to,
                                                const std::string &message) {
  const Message pubMessage =
      MessageBuilder::createPubMessage(subject, reply_to, message);
  sendNatsMessage(pubMessage);
}

// TODO(talnordan): Consider introducing `BufferUtility` and extracting this
// member function into it.
std::string ClientImpl::drainBufferToString(Buffer::Instance &buffer) const {
  std::string output = bufferToString(buffer);
  buffer.drain(buffer.length());
  return output;
}

// TODO(talnordan): This is duplicated from `TestUtility::bufferToString()`.
// Consider moving the code to a shared utility class.
// TODO(talnordan): Consider leveraging the fact that `max_payload` is given in
// the NATS `INFO` message and reuse the same pre-allocated `std:string`.
std::string ClientImpl::bufferToString(const Buffer::Instance &buffer) const {
  std::string output;
  uint64_t num_slices = buffer.getRawSlices(nullptr, 0);
  Buffer::RawSlice slices[num_slices];
  buffer.getRawSlices(slices, num_slices);
  for (Buffer::RawSlice &slice : slices) {
    output.append(static_cast<const char *>(slice.mem_), slice.len_);
  }

  return output;
}

} // namespace Streaming
} // namespace Nats
} // namespace Envoy
