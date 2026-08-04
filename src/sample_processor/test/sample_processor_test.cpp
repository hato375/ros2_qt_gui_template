#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>

#include <rclcpp/rclcpp.hpp>
#include <yds_interfaces/msg/component_status.hpp>

#include <sample_processor/sample_processor_node.h>
#include <yds/ros2/executor_runner.h>

namespace {

using namespace std::chrono_literals;

TEST(SampleProcessorNodeTest, PublishesReadyAndRunningStatus) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter("processing_interval_ms", 100),
		rclcpp::Parameter("component_status.publish_interval_ms", 100),
	});
	auto node = std::make_shared<sampleprocessor::SampleProcessorNode>(options);
	std::mutex receivedMutex;
	yds_interfaces::msg::ComponentStatus latestStatus;
	bool receivedRunning = false;
	const auto subscription = node->create_subscription<yds_interfaces::msg::ComponentStatus>(
		"sample_processor/status",
		rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local(),
		[&](const yds_interfaces::msg::ComponentStatus::SharedPtr message) {
			std::lock_guard<std::mutex> lock(receivedMutex);
			latestStatus = *message;
			if (message->state == yds_interfaces::msg::ComponentStatus::STATE_RUNNING) {
				receivedRunning = true;
			}
		});
	ASSERT_NE(subscription, nullptr);
	yds::ros2::ExecutorRunner executorRunner(node);

	const auto deadline = std::chrono::steady_clock::now() + 2s;
	while (std::chrono::steady_clock::now() < deadline) {
		{
			std::lock_guard<std::mutex> lock(receivedMutex);
			if (receivedRunning) {
				break;
			}
		}
		std::this_thread::sleep_for(10ms);
	}
	executorRunner.stop();

	std::lock_guard<std::mutex> lock(receivedMutex);
	EXPECT_TRUE(receivedRunning);
	EXPECT_EQ(latestStatus.component_id, "sample-processor-1");
	EXPECT_EQ(latestStatus.state, yds_interfaces::msg::ComponentStatus::STATE_RUNNING);
	EXPECT_EQ(node->processingIntervalMs(), 100);
	EXPECT_GE(node->processedCount(), 1U);
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
