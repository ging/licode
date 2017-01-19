#include "rtp/RRGenerationHandler.h"
#include "./WebRtcConnection.h"
#include "lib/ClockUtils.h"

namespace erizo {

DEFINE_LOGGER(RRGenerationHandler, "rtp.RRGenerationHandler");

RRGenerationHandler::RRGenerationHandler(WebRtcConnection *connection) :
    connection_{connection}, temp_ctx_{nullptr}, enabled_{true} {}


void RRGenerationHandler::enable() {
  enabled_ = true;
}

void RRGenerationHandler::disable() {
  enabled_ = false;
}

bool RRGenerationHandler::rtpSequenceLessThan(uint16_t x, uint16_t y) {
  int diff = y - x;
  if (diff > 0) {
    return (diff < 0x8000);
  } else if (diff < 0) {
    return (diff < -0x8000);
  } else {  // diff == 0
    return false;
  }
}

// TODO(kekkokk) Consider add more payload types
int RRGenerationHandler::getAudioClockRate(uint8_t payloadType) {
  if (payloadType == OPUS_48000_PT) {
    return 48;
  } else {
    return 8;
  }
}

// TODO(kekkokk) ATM we only use 90Khz video type so always return 90
int RRGenerationHandler::getVideoClockRate(uint8_t payloadType) {
  return 90;
}

void RRGenerationHandler::handleRtpPacket(std::shared_ptr<dataPacket> packet) {
  RtpHeader *head = reinterpret_cast<RtpHeader*>(packet->data);
  uint16_t seq_num = head->getSeqNumber();
  uint32_t now = ClockUtils::timePointToMs(clock::now());

  switch (packet->type) {
  case VIDEO_PACKET: {
    video_rr_.p_received++;
    if (video_rr_.base_seq == 0) {
      video_rr_.ssrc = head->getSSRC();
      video_rr_.base_seq = head->getSeqNumber();
    }
    if (!rtpSequenceLessThan(seq_num, video_rr_.max_seq)) {
      if (seq_num < video_rr_.max_seq) {
        video_rr_.cycle++;
      }
      video_rr_.max_seq = seq_num;
    }
    video_rr_.extended_max = (video_rr_.cycle << 16) | video_rr_.max_seq;
    int clock_rate = getVideoClockRate(head->getPayloadType());
    if (head->getTimestamp() != video_rr_.last_rtp_ts) {
      int transit = static_cast<int>((now * clock_rate) - head->getTimestamp());
      int delta = abs(transit - jitter_video_.transit);
      if (jitter_video_.transit != 0 && delta < 450000) {
        jitter_video_.jitter += (1. / 16.) * (static_cast<double>(delta) - jitter_video_.jitter);
      }
      jitter_video_.transit = transit;
    }
    video_rr_.last_rtp_ts = head->getTimestamp();
    break;
  }
  case AUDIO_PACKET: {
    audio_rr_.p_received++;
    if (audio_rr_.base_seq == 0) {
      audio_rr_.ssrc = head->getSSRC();
      audio_rr_.base_seq = head->getSeqNumber();
    }
    if (!rtpSequenceLessThan(seq_num, audio_rr_.max_seq)) {
      if (seq_num < audio_rr_.max_seq) {
        audio_rr_.cycle++;
      }
      audio_rr_.max_seq = seq_num;
    }
    audio_rr_.extended_max = (audio_rr_.cycle << 16) | audio_rr_.max_seq;
    int clock_rate = getAudioClockRate(head->getPayloadType());
    if (head->getTimestamp() != audio_rr_.last_rtp_ts) {
      int transit = static_cast<int>((now * clock_rate) - head->getTimestamp());
      int delta = abs(transit - jitter_audio_.transit);
      if (jitter_audio_.transit != 0 && delta < 450000) {
        jitter_audio_.jitter += (1. / 16.) * (static_cast<double>(delta) - jitter_audio_.jitter);
      }
      jitter_audio_.transit = transit;
    }
    audio_rr_.last_rtp_ts = head->getTimestamp();
    break;
  }
  default:
    break;
  }
}

void RRGenerationHandler::sendVideoRR() {
  int64_t now = ClockUtils::timePointToMs(clock::now());

  if (video_rr_.ssrc != 0 && enabled_) {
    RtcpHeader rtcp_head;
    rtcp_head.setPacketType(RTCP_Receiver_PT);
    rtcp_head.setSSRC(video_rr_.ssrc);
    rtcp_head.setSourceSSRC(video_rr_.ssrc);
    rtcp_head.setHighestSeqnum(video_rr_.extended_max);
    rtcp_head.setSeqnumCycles(video_rr_.cycle);
    rtcp_head.setLostPackets(video_rr_.lost);
    rtcp_head.setFractionLost(static_cast<uint8_t>(video_rr_.frac_lost));
    rtcp_head.setJitter(static_cast<uint32_t>(jitter_video_.jitter));
    rtcp_head.setDelaySinceLastSr(now - video_rr_.last_sr_recv_ts);
    rtcp_head.setLastSr(video_rr_.last_sr_mid_ntp);
    rtcp_head.setLength(7);
    rtcp_head.setBlockCount(1);
    int length = (rtcp_head.getLength() + 1) * 4;
    memcpy(packet_, reinterpret_cast<uint8_t*>(&rtcp_head), length);
    if (temp_ctx_) {
      temp_ctx_->fireWrite(std::make_shared<dataPacket>(0, reinterpret_cast<char*>(&packet_), length, OTHER_PACKET));
      video_rr_.last_rr_sent_ts = now;
      ELOG_WARN("VIDEO JITTER: %u - lost: %u - frac: %u", rtcp_head.getJitter(), video_rr_.lost, video_rr_.frac_lost);
    }
  }
}

void RRGenerationHandler::sendAudioRR() {
  int64_t now = ClockUtils::timePointToMs(clock::now());

  if (audio_rr_.ssrc != 0 && enabled_) {
    RtcpHeader rtcp_head;
    rtcp_head.setPacketType(RTCP_Receiver_PT);
    rtcp_head.setSSRC(audio_rr_.ssrc);
    rtcp_head.setSourceSSRC(audio_rr_.ssrc);
    rtcp_head.setHighestSeqnum(audio_rr_.extended_max);
    rtcp_head.setSeqnumCycles(audio_rr_.cycle);
    rtcp_head.setLostPackets(audio_rr_.lost);
    rtcp_head.setFractionLost(static_cast<uint8_t>(audio_rr_.frac_lost));
    rtcp_head.setJitter(static_cast<uint32_t>(jitter_audio_.jitter));
    rtcp_head.setDelaySinceLastSr(now - audio_rr_.last_sr_recv_ts);
    rtcp_head.setLastSr(audio_rr_.last_sr_mid_ntp);
    rtcp_head.setLength(7);
    rtcp_head.setBlockCount(1);
    int length = (rtcp_head.getLength() + 1) * 4;
    memcpy(packet_, reinterpret_cast<uint8_t*>(&rtcp_head), length);
    if (temp_ctx_) {
      temp_ctx_->fireWrite(std::make_shared<dataPacket>(0, reinterpret_cast<char*>(&packet_), length, OTHER_PACKET));
      audio_rr_.last_rr_sent_ts = now;
      ELOG_WARN("AUDIO JITTER: %u - lost: %u - frac: %u", rtcp_head.getJitter(), audio_rr_.lost, audio_rr_.frac_lost);
    }
  }
}

void RRGenerationHandler::handleSR(std::shared_ptr<dataPacket> packet) {
  RtcpHeader *chead = reinterpret_cast<RtcpHeader*>(packet->data);

    if (chead->getSSRC() == video_rr_.ssrc) {
      video_rr_.last_sr_mid_ntp = chead->get32MiddleNtp();
      video_rr_.last_sr_recv_ts = packet->received_time_ms;
      uint32_t expected = video_rr_.extended_max- video_rr_.base_seq + 1;
      video_rr_.lost = expected - video_rr_.p_received;
      uint32_t fraction = 0;
      uint32_t expected_interval = expected - video_rr_.expected_prior;
      video_rr_.expected_prior = expected;
      uint32_t received_interval = video_rr_.p_received - video_rr_.received_prior;
      video_rr_.received_prior = video_rr_.p_received;
      uint32_t lost_interval = expected_interval - received_interval;
      if (expected_interval != 0 && lost_interval > 0) {
        fraction = (lost_interval << 8) / expected_interval;
      }
      video_rr_.frac_lost = fraction;
      sendVideoRR();
    }

    if (chead->getSSRC() == audio_rr_.ssrc) {
      audio_rr_.last_sr_mid_ntp = chead->get32MiddleNtp();
      audio_rr_.last_sr_recv_ts = packet->received_time_ms;
      uint32_t expected = audio_rr_.extended_max- audio_rr_.base_seq + 1;
      audio_rr_.lost = expected - audio_rr_.p_received;
      uint32_t fraction = 0;
      uint32_t expected_interval = expected - audio_rr_.expected_prior;
      audio_rr_.expected_prior = expected;
      uint32_t received_interval = audio_rr_.p_received - audio_rr_.received_prior;
      audio_rr_.received_prior = audio_rr_.p_received;
      uint32_t lost_interval = expected_interval - received_interval;
      if (expected_interval != 0 && lost_interval > 0) {
        fraction = (lost_interval << 8) / expected_interval;
      }
      audio_rr_.frac_lost = fraction;
      sendAudioRR();
    }
}

void RRGenerationHandler::read(Context *ctx, std::shared_ptr<dataPacket> packet) {
  temp_ctx_ = ctx;

  RtcpHeader *chead = reinterpret_cast<RtcpHeader*>(packet->data);
  if (!chead->isRtcp()) {
    handleRtpPacket(packet);
  } else if (chead->packettype == RTCP_Sender_PT) {
    handleSR(packet);
  }
  ctx->fireRead(packet);
}

void RRGenerationHandler::write(Context *ctx, std::shared_ptr<dataPacket> packet) {
  ctx->fireWrite(packet);
}

}  // namespace erizo
