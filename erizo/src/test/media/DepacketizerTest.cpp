#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <media/Depacketizer.h>

#include "../utils/Mocks.h"
#include "../utils/Tools.h"
#include "../utils/Matchers.h"

namespace {
  constexpr uint16_t seq = 44444;

  char change_bit(char val, int num, bool bitval) {
    return (val & ~(1 << num)) | (bitval << num);
  }
}

class DepacketizerVp8Test : public erizo::HandlerTest {
 public:
  DepacketizerVp8Test() {}

 protected:
  void setHandler() {
    depacketizer = std::make_shared<erizo::Vp8_depacketizer>();
  }

  int Vp8PacketDataSize() {
    const auto pkt = erizo::PacketTools::createVP8Packet(seq, false, false);
    const erizo::RtpHeader* head = reinterpret_cast<const erizo::RtpHeader*>(pkt->data);
    return pkt->length - head->getHeaderLength() - 1;  // subtract 1 to skip vp8 header
  }

  std::shared_ptr<erizo::Depacketizer> depacketizer;
};

TEST_F(DepacketizerVp8Test, isReallyEmpty) {
  EXPECT_EQ(0, depacketizer->is_keyframe());
  EXPECT_EQ(0, depacketizer->frame_size());
}

TEST_F(DepacketizerVp8Test, shouldDepacketSimplePacket) {
  const auto pkt = erizo::PacketTools::createVP8Packet(seq, false, true);
  depacketizer->fetch_packet(reinterpret_cast<unsigned char*>(pkt->data), pkt->length);
  EXPECT_EQ(1, depacketizer->process_packet());
  EXPECT_EQ(0, depacketizer->is_keyframe());
  EXPECT_EQ(Vp8PacketDataSize(), depacketizer->frame_size());
}

TEST_F(DepacketizerVp8Test, shouldDepacketSimplePacketFrame) {
  const auto pkt = erizo::PacketTools::createVP8Packet(seq, false, true);
  depacketizer->fetch_packet(reinterpret_cast<unsigned char*>(pkt->data), pkt->length);
  EXPECT_EQ(1, depacketizer->process_packet());
  EXPECT_EQ(0, depacketizer->is_keyframe());
  EXPECT_EQ(Vp8PacketDataSize(), depacketizer->frame_size());
}

TEST_F(DepacketizerVp8Test, shouldDepacketSimplePacketKeyframe) {
  const auto pkt = erizo::PacketTools::createVP8Packet(seq, true, true);
  depacketizer->fetch_packet(reinterpret_cast<unsigned char*>(pkt->data), pkt->length);
  EXPECT_EQ(1, depacketizer->process_packet());
  EXPECT_EQ(1, depacketizer->is_keyframe());
  EXPECT_EQ(Vp8PacketDataSize(), depacketizer->frame_size());
}

TEST_F(DepacketizerVp8Test, shouldDepacketMultiPacketFrame) {
  auto pkt = erizo::PacketTools::createVP8Packet(seq, false, false);
  depacketizer->fetch_packet(reinterpret_cast<unsigned char*>(pkt->data), pkt->length);
  EXPECT_EQ(0, depacketizer->process_packet());
  EXPECT_EQ(0, depacketizer->is_keyframe());

  pkt = erizo::PacketTools::createVP8Packet(seq + 1, false, false);
  // unset the bit indicating the start of partition
  unsigned char* pkt_ptr = reinterpret_cast<unsigned char*>(pkt->data);
  erizo::RtpHeader* head = reinterpret_cast<erizo::RtpHeader*>(pkt_ptr);
  pkt_ptr += head->getHeaderLength();
  *pkt_ptr = change_bit(*pkt_ptr, 4, false);
  depacketizer->fetch_packet(reinterpret_cast<unsigned char*>(pkt->data), pkt->length);
  EXPECT_EQ(0, depacketizer->process_packet());
  EXPECT_EQ(0, depacketizer->is_keyframe());

  pkt = erizo::PacketTools::createVP8Packet(seq + 2, false, true);
  // unset the bit indicating the start of partition
  pkt_ptr = reinterpret_cast<unsigned char*>(pkt->data);
  head = reinterpret_cast<erizo::RtpHeader*>(pkt_ptr);
  pkt_ptr += head->getHeaderLength();
  *pkt_ptr = change_bit(*pkt_ptr, 4, false);
  depacketizer->fetch_packet(reinterpret_cast<unsigned char*>(pkt->data), pkt->length);
  EXPECT_EQ(1, depacketizer->process_packet());
  EXPECT_EQ(0, depacketizer->is_keyframe());
  EXPECT_EQ(Vp8PacketDataSize() * 3, depacketizer->frame_size());
}

TEST_F(DepacketizerVp8Test, shouldReset) {
  const auto pkt = erizo::PacketTools::createVP8Packet(seq, true, true);
  depacketizer->fetch_packet(reinterpret_cast<unsigned char*>(pkt->data), pkt->length);
  EXPECT_EQ(1, depacketizer->process_packet());
  depacketizer->reset();
  EXPECT_EQ(0, depacketizer->is_keyframe());
  EXPECT_EQ(0, depacketizer->frame_size());
}
