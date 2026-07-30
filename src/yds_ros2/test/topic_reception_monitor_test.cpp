#include <chrono>
#include <stdexcept>
#include <thread>

#include <gtest/gtest.h>

#include "yds/ros2/topic_reception_monitor.h"

namespace {

using namespace std::chrono_literals;

TEST(TopicReceptionMonitorTest, RejectsInvalidConfiguration) {
	EXPECT_THROW(
		yds::ros2::TopicReceptionMonitor(QString(), 100ms),
		std::invalid_argument);
	EXPECT_THROW(
		yds::ros2::TopicReceptionMonitor(QStringLiteral("status"), 0ms),
		std::invalid_argument);
}

TEST(TopicReceptionMonitorTest, ReportsStartedTimeoutAndRecoveredTransitions) {
	yds::ros2::TopicReceptionMonitor monitor(QStringLiteral("status"), 20ms);

	EXPECT_EQ(
		monitor.recordReception(QStringLiteral("ready")),
		yds::ros2::TopicReceptionTransition::kStarted);
	auto status = monitor.takeStatusUpdate();
	ASSERT_TRUE(status.has_value());
	EXPECT_EQ(status->topicName, QStringLiteral("status"));
	EXPECT_EQ(status->state, yds::ros2::TopicReceptionState::kReceiving);
	EXPECT_EQ(status->receivedCount, 1U);
	EXPECT_EQ(status->lastMessage, QStringLiteral("ready"));
	EXPECT_FALSE(monitor.takeStatusUpdate().has_value());

	std::this_thread::sleep_for(30ms);
	EXPECT_EQ(
		monitor.checkTimeout(),
		yds::ros2::TopicReceptionTransition::kTimedOut);
	status = monitor.takeStatusUpdate();
	ASSERT_TRUE(status.has_value());
	EXPECT_EQ(status->state, yds::ros2::TopicReceptionState::kTimedOut);
	EXPECT_EQ(
		monitor.checkTimeout(),
		yds::ros2::TopicReceptionTransition::kNone);

	EXPECT_EQ(
		monitor.recordReception(QStringLiteral("running")),
		yds::ros2::TopicReceptionTransition::kRecovered);
	status = monitor.takeStatusUpdate();
	ASSERT_TRUE(status.has_value());
	EXPECT_EQ(status->state, yds::ros2::TopicReceptionState::kReceiving);
	EXPECT_EQ(status->receivedCount, 2U);
	EXPECT_EQ(status->lastMessage, QStringLiteral("running"));
}

TEST(TopicReceptionMonitorTest, KeepsOnlyLatestPendingStatus) {
	yds::ros2::TopicReceptionMonitor monitor(QStringLiteral("status"), 1000ms);

	for (int index = 0; index < 10; ++index) {
		monitor.recordReception(QStringLiteral("message-%1").arg(index));
	}

	const auto status = monitor.takeStatusUpdate();
	ASSERT_TRUE(status.has_value());
	EXPECT_EQ(status->receivedCount, 10U);
	EXPECT_EQ(status->lastMessage, QStringLiteral("message-9"));
	EXPECT_FALSE(monitor.takeStatusUpdate().has_value());
}

}  // namespace
