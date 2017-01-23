#ifndef ERIZO_SRC_ERIZO_RTP_FECRECEIVERHANDLER_H_
#define ERIZO_SRC_ERIZO_RTP_FECRECEIVERHANDLER_H_

#include <string>

#include "./logger.h"
#include "pipeline/Handler.h"
#include "webrtc/modules/rtp_rtcp/include/ulpfec_receiver.h"

namespace erizo {

class WebRtcConnection;

class FecReceiverHandler: public OutboundHandler, public webrtc::RtpData {
  DECLARE_LOGGER();

 public:
  explicit FecReceiverHandler(WebRtcConnection* connection);

  void enable() override;
  void disable() override;

  std::string getName() override {
    return "fec-receiver";
  }

  void write(Context *ctx, std::shared_ptr<dataPacket> packet) override;

  // webrtc::RtpHeader overrides.
  int32_t OnReceivedPayloadData(const uint8_t* payloadData, size_t payloadSize,
                                const webrtc::WebRtcRTPHeader* rtpHeader) override;
  bool OnRecoveredPacket(const uint8_t* packet, size_t packet_length) override;

 private:
  WebRtcConnection* connection_;
  bool enabled_;
  std::unique_ptr<webrtc::UlpfecReceiver> fec_receiver_;
};

}  // namespace erizo

#endif  // ERIZO_SRC_ERIZO_RTP_FECRECEIVERHANDLER_H_
