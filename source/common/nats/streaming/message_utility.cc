#include "common/nats/streaming/message_utility.h"

#include "protocol.pb.h"

namespace Envoy {
namespace Nats {
namespace Streaming {

std::string MessageUtility::createConnectRequestMessage(
    const std::string &client_id, const std::string &heartbeat_inbox) const {
  pb::ConnectRequest connect_request;
  connect_request.set_clientid(client_id);
  connect_request.set_heartbeatinbox(heartbeat_inbox);

  return serializeToString(connect_request);
}

std::string MessageUtility::createConnectResponseMessage(
    const std::string &pub_prefix, const std::string &sub_requests,
    const std::string &unsub_requests,
    const std::string &close_requests) const {
  pb::ConnectResponse connect_response;
  connect_response.set_pubprefix(pub_prefix);
  connect_response.set_subrequests(sub_requests);
  connect_response.set_unsubrequests(unsub_requests);
  connect_response.set_closerequests(close_requests);

  return serializeToString(connect_response);
}

std::string MessageUtility::createPubMsgMessage(const std::string &client_id,
                                                const std::string &guid,
                                                const std::string &subject,
                                                const std::string &data) const {
  pb::PubMsg pub_msg;
  pub_msg.set_clientid(client_id);
  pub_msg.set_guid(guid);
  pub_msg.set_subject(subject);
  pub_msg.set_data(data);

  return serializeToString(pub_msg);
}

std::string MessageUtility::getPubPrefix(
    const std::string &connect_response_message) const {
  pb::ConnectResponse connect_response;
  connect_response.ParseFromString(connect_response_message);
  return connect_response.pubprefix();
}

} // namespace Streaming
} // namespace Nats
} // namespace Envoy
