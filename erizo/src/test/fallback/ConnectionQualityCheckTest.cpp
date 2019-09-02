#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <rtp/RtpHeaders.h>
#include <rtp/RtpUtils.h>
#include <MediaDefinitions.h>
#include <bandwidth/ConnectionQualityCheck.h>

#include <string>
#include <tuple>

#include "../utils/Mocks.h"
#include "../utils/Matchers.h"

using testing::_;
using testing::Return;
using testing::Eq;
using testing::Args;
using testing::AtLeast;
using erizo::DataPacket;
using erizo::ExtMap;
using erizo::IceConfig;
using erizo::RtpMap;
using erizo::RtpUtils;
using erizo::MediaStream;
using erizo::ConnectionQualityCheck;
using erizo::ConnectionQualityLevel;

using std::make_tuple;

typedef std::vector<int16_t> FractionLostList;

class BasicConnectionQualityCheckTest {
 protected:
  void setUpStreams() {
    simulated_clock = std::make_shared<erizo::SimulatedClock>();
    simulated_worker = std::make_shared<erizo::SimulatedWorker>(simulated_clock);
    simulated_worker->start();
    for (uint32_t index = 0; index < fraction_lost_list.size(); index++) {
      auto mock_stream = addMediaStream(false, index);
      auto erizo_stream = std::static_pointer_cast<erizo::MediaStream>(mock_stream);
      streams.push_back(mock_stream);
      erizo_streams.push_back(erizo_stream);
    }
  }

  std::shared_ptr<erizo::MockMediaStream> addMediaStream(bool is_publisher, uint32_t index) {
    std::string id = std::to_string(index);
    std::string label = std::to_string(index);
    uint32_t video_sink_ssrc = getSsrcFromIndex(index);
    uint32_t audio_sink_ssrc = getSsrcFromIndex(index) + 1;
    uint32_t video_source_ssrc = getSsrcFromIndex(index) + 2;
    uint32_t audio_source_ssrc = getSsrcFromIndex(index) + 3;
    auto media_stream = std::make_shared<erizo::MockMediaStream>(simulated_worker, nullptr, id, label,
     rtp_maps, is_publisher);
    media_stream->setVideoSinkSSRC(video_sink_ssrc);
    media_stream->setAudioSinkSSRC(audio_sink_ssrc);
    media_stream->setVideoSourceSSRC(video_source_ssrc);
    media_stream->setAudioSourceSSRC(audio_source_ssrc);

    return media_stream;
  }

  void onFeedbackReceived() {
    uint32_t index = 0;
    for (int16_t fraction_lost : fraction_lost_list) {
      if (fraction_lost >= 0) {
        uint8_t f_lost = fraction_lost  * 256 / 100;
        auto packet = RtpUtils::createReceiverReport(getSsrcFromIndex(index) + 1, f_lost);
        connection_quality_check->onFeedback(packet, erizo_streams);
      }
      index++;
    }
    simulated_worker->executeTasks();
  }

  uint32_t getIndexFromSsrc(uint32_t ssrc) {
    return ssrc / 1000;
  }

  uint32_t getSsrcFromIndex(uint32_t index) {
    return index * 1000;
  }

  std::shared_ptr<erizo::SimulatedClock> simulated_clock;
  std::shared_ptr<erizo::SimulatedWorker> simulated_worker;
  std::vector<std::shared_ptr<erizo::MockMediaStream>> streams;
  std::vector<std::shared_ptr<erizo::MediaStream>> erizo_streams;
  FractionLostList fraction_lost_list;
  ConnectionQualityLevel expected_quality_level;
  std::vector<RtpMap> rtp_maps;
  std::vector<ExtMap> ext_maps;
  std::shared_ptr<ConnectionQualityCheck> connection_quality_check;
};

class ConnectionQualityCheckTest : public BasicConnectionQualityCheckTest,
  public ::testing::TestWithParam<std::tr1::tuple<FractionLostList,
                                                  ConnectionQualityLevel>> {
 protected:
  virtual void SetUp() {
    fraction_lost_list = std::tr1::get<0>(GetParam());
    expected_quality_level = std::tr1::get<1>(GetParam());

    connection_quality_check = std::make_shared<erizo::ConnectionQualityCheck>();

    setUpStreams();
  }

  virtual void TearDown() {
    streams.clear();
  }
};

TEST_P(ConnectionQualityCheckTest, notifyConnectionQualityEvent_When_ItChanges) {
  std::for_each(streams.begin(), streams.end(), [this](const std::shared_ptr<erizo::MockMediaStream> &stream) {
    if (expected_quality_level != ConnectionQualityLevel::GOOD) {
      EXPECT_CALL(*stream, deliverEventInternal(_)).
        With(Args<0>(erizo::ConnectionQualityEventWithLevel(expected_quality_level))).Times(1);
    } else {
      EXPECT_CALL(*stream, deliverEventInternal(_)).Times(0);
    }
  });


  onFeedbackReceived();
}

INSTANTIATE_TEST_CASE_P(
  FractionLost_Values, ConnectionQualityCheckTest, testing::Values(
    //                          fraction_losts (%)   expected_quality_level
    make_tuple(FractionLostList{ 99, 99, 99, 99},     ConnectionQualityLevel::HIGH_AUDIO_LOSSES),
    make_tuple(FractionLostList{ 25, 25, 25, 25},     ConnectionQualityLevel::HIGH_AUDIO_LOSSES),
    make_tuple(FractionLostList{  0,  0, 41, 41},     ConnectionQualityLevel::HIGH_AUDIO_LOSSES),
    make_tuple(FractionLostList{ 19, 19, 19, 19},     ConnectionQualityLevel::LOW_AUDIO_LOSSES),
    make_tuple(FractionLostList{ 10, 10, 10, 10},     ConnectionQualityLevel::LOW_AUDIO_LOSSES),
    make_tuple(FractionLostList{  0,  0, 20, 20},     ConnectionQualityLevel::LOW_AUDIO_LOSSES),
    make_tuple(FractionLostList{  4,  4,  4,  4},     ConnectionQualityLevel::GOOD),
    make_tuple(FractionLostList{  0,  0,  0,  0},     ConnectionQualityLevel::GOOD),
    make_tuple(FractionLostList{ -1, 99, 99, 99},     ConnectionQualityLevel::GOOD)
));
