#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>
#include <QString>

#include <rclcpp/rclcpp.hpp>
#include <yds_interfaces/msg/component_status.hpp>

#include <yds/ros2/component_status_node.h>
#include <yds/ros2/executor_runner.h>

namespace {

using namespace std::chrono_literals;

class TestComponentStatusNode final : public yds::ros2::ComponentStatusNode {
public:
	explicit TestComponentStatusNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions())
		: ComponentStatusNode(
			"component_status_node_test",
			QStringLiteral("camera-1"),
			QStringLiteral("camera/status"),
			100ms,
			options) {}
};

TEST(ComponentStatusNodeTest, UsesParameterOverrides) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter("component_status.component_id", "plc-1"),
		rclcpp::Parameter("component_status.status_topic", "plc/status"),
		rclcpp::Parameter("component_status.publish_interval_ms", 250)});
	const auto node = std::make_shared<TestComponentStatusNode>(options);

	EXPECT_EQ(node->componentId(), QStringLiteral("plc-1"));
	EXPECT_EQ(node->statusTopicName(), QStringLiteral("plc/status"));
	EXPECT_EQ(node->statusPublishInterval(), 250ms);
	EXPECT_EQ(node->componentStatus().state, yds::ros2::ComponentState::kInitializing);
}

TEST(ComponentStatusNodeTest, RejectsInvalidPublishInterval) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter("component_status.publish_interval_ms", 99)});

	EXPECT_THROW(std::make_shared<TestComponentStatusNode>(options), std::out_of_range);
}

TEST(ComponentStatusNodeTest, PublishesUpdatedStatusAndPeriodicHeartbeat) {
	auto node = std::make_shared<TestComponentStatusNode>();
	std::mutex receivedMutex;
	int receivedCount = 0;
	yds_interfaces::msg::ComponentStatus latestMessage;
	const auto subscription = node->create_subscription<yds_interfaces::msg::ComponentStatus>(
		"camera/status",
		rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local(),
		[&](const yds_interfaces::msg::ComponentStatus::SharedPtr message) {
			std::lock_guard<std::mutex> lock(receivedMutex);
			latestMessage = *message;
			++receivedCount;
		});
	ASSERT_NE(subscription, nullptr);
	yds::ros2::ExecutorRunner executorRunner(node);

	EXPECT_TRUE(node->setComponentStatus(
		yds::ros2::ComponentState::kRunning,
		12,
		QStringLiteral("Processing")));
	const auto deadline = std::chrono::steady_clock::now() + 2s;
	while (std::chrono::steady_clock::now() < deadline) {
		{
			std::lock_guard<std::mutex> lock(receivedMutex);
			if (receivedCount >= 2 &&
				latestMessage.state == yds_interfaces::msg::ComponentStatus::STATE_RUNNING) {
				break;
			}
		}
		std::this_thread::sleep_for(10ms);
	}
	executorRunner.stop();

	std::lock_guard<std::mutex> lock(receivedMutex);
	EXPECT_GE(receivedCount, 2);
	EXPECT_EQ(latestMessage.component_id, "camera-1");
	EXPECT_EQ(latestMessage.state, yds_interfaces::msg::ComponentStatus::STATE_RUNNING);
	EXPECT_EQ(latestMessage.error_code, 12);
	EXPECT_EQ(latestMessage.message, "Processing");
	EXPECT_NE(latestMessage.header.stamp.sec, 0);
}

}  // namespace
