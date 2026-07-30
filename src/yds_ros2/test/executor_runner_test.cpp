#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>

#include "yds/ros2/application_event.h"
#include "yds/ros2/executor_runner.h"
#include "yds/ros2/topic_reception_status.h"

namespace {

using namespace std::chrono_literals;

bool waitFor(const std::atomic<std::uint64_t>& count, std::chrono::milliseconds timeout) {
	const auto deadline = std::chrono::steady_clock::now() + timeout;
	while (count.load() == 0 && std::chrono::steady_clock::now() < deadline) {
		std::this_thread::sleep_for(5ms);
	}
	return count.load() > 0;
}

TEST(ExecutorRunnerTest, ExecutesCallbacksAndStopsSafely) {
	auto node = std::make_shared<rclcpp::Node>("yds_ros2_executor_runner_test");
	std::atomic<std::uint64_t> callbackCount(0);
	auto timer = node->create_wall_timer(10ms, [&callbackCount]() {
		++callbackCount;
	});

	yds::ros2::ExecutorRunner executorRunner(node);

	EXPECT_TRUE(waitFor(callbackCount, 1000ms));
	executorRunner.stop();
	executorRunner.stop();
}

TEST(ApplicationEventTest, ConvertsEventLevelsToText) {
	EXPECT_EQ(
		yds::ros2::eventLevelText(yds::ros2::ApplicationEventLevel::kInfo),
		QStringLiteral("INFO"));
	EXPECT_EQ(
		yds::ros2::eventLevelText(yds::ros2::ApplicationEventLevel::kWarning),
		QStringLiteral("WARN"));
	EXPECT_EQ(
		yds::ros2::eventLevelText(yds::ros2::ApplicationEventLevel::kError),
		QStringLiteral("ERROR"));
	EXPECT_EQ(
		yds::ros2::eventLevelText(yds::ros2::ApplicationEventLevel::kCritical),
		QStringLiteral("CRITICAL"));
	EXPECT_EQ(
		yds::ros2::eventLevelText(static_cast<yds::ros2::ApplicationEventLevel>(-1)),
		QStringLiteral("UNKNOWN"));
}

TEST(TopicReceptionStatusTest, ConvertsStatesToText) {
	EXPECT_EQ(
		yds::ros2::topicReceptionStateText(yds::ros2::TopicReceptionState::kWaiting),
		QStringLiteral("WAITING"));
	EXPECT_EQ(
		yds::ros2::topicReceptionStateText(yds::ros2::TopicReceptionState::kReceiving),
		QStringLiteral("RECEIVING"));
	EXPECT_EQ(
		yds::ros2::topicReceptionStateText(yds::ros2::TopicReceptionState::kTimedOut),
		QStringLiteral("TIMED OUT"));
	EXPECT_EQ(
		yds::ros2::topicReceptionStateText(static_cast<yds::ros2::TopicReceptionState>(-1)),
		QStringLiteral("UNKNOWN"));
}

}  // namespace

int main(int argc, char* argv[]) {
	testing::InitGoogleTest(&argc, argv);

	int rosArgumentCount = 0;
	char** rosArguments = nullptr;
	rclcpp::init(rosArgumentCount, rosArguments);
	const int result = RUN_ALL_TESTS();
	rclcpp::shutdown();
	return result;
}
