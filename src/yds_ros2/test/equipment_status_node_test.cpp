#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>
#include <QString>

#include <rclcpp/rclcpp.hpp>
#include <yds_interfaces/msg/equipment_status.hpp>

#include <yds/ros2/equipment_status_node.h>
#include <yds/ros2/executor_runner.h>

namespace {

using namespace std::chrono_literals;

class TestEquipmentStatusNode final : public yds::ros2::EquipmentStatusNode {
public:
	explicit TestEquipmentStatusNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions())
		: EquipmentStatusNode(
			"equipment_status_node_test",
			QStringLiteral("camera-1"),
			QStringLiteral("camera/status"),
			100ms,
			options) {}
};

TEST(EquipmentStatusNodeTest, UsesParameterOverrides) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter("equipment_status.equipment_id", "plc-1"),
		rclcpp::Parameter("equipment_status.topic_name", "plc/status"),
		rclcpp::Parameter("equipment_status.publish_interval_ms", 250)});
	const auto node = std::make_shared<TestEquipmentStatusNode>(options);

	EXPECT_EQ(node->equipmentId(), QStringLiteral("plc-1"));
	EXPECT_EQ(node->statusTopicName(), QStringLiteral("plc/status"));
	EXPECT_EQ(node->statusPublishInterval(), 250ms);
	EXPECT_EQ(node->equipmentStatus().state, yds::ros2::EquipmentState::kInitializing);
}

TEST(EquipmentStatusNodeTest, RejectsInvalidPublishInterval) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter("equipment_status.publish_interval_ms", 99)});

	EXPECT_THROW(std::make_shared<TestEquipmentStatusNode>(options), std::out_of_range);
}

TEST(EquipmentStatusNodeTest, PublishesUpdatedStatusAndPeriodicHeartbeat) {
	auto node = std::make_shared<TestEquipmentStatusNode>();
	std::mutex receivedMutex;
	int receivedCount = 0;
	yds_interfaces::msg::EquipmentStatus latestMessage;
	const auto subscription = node->create_subscription<yds_interfaces::msg::EquipmentStatus>(
		"camera/status",
		rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local(),
		[&](const yds_interfaces::msg::EquipmentStatus::SharedPtr message) {
			std::lock_guard<std::mutex> lock(receivedMutex);
			latestMessage = *message;
			++receivedCount;
		});
	ASSERT_NE(subscription, nullptr);
	yds::ros2::ExecutorRunner executorRunner(node);

	EXPECT_TRUE(node->setEquipmentStatus(
		yds::ros2::EquipmentState::kRunning,
		12,
		QStringLiteral("Processing")));
	const auto deadline = std::chrono::steady_clock::now() + 2s;
	while (std::chrono::steady_clock::now() < deadline) {
		{
			std::lock_guard<std::mutex> lock(receivedMutex);
			if (receivedCount >= 2 &&
				latestMessage.state == yds_interfaces::msg::EquipmentStatus::STATE_RUNNING) {
				break;
			}
		}
		std::this_thread::sleep_for(10ms);
	}
	executorRunner.stop();

	std::lock_guard<std::mutex> lock(receivedMutex);
	EXPECT_GE(receivedCount, 2);
	EXPECT_EQ(latestMessage.equipment_id, "camera-1");
	EXPECT_EQ(latestMessage.state, yds_interfaces::msg::EquipmentStatus::STATE_RUNNING);
	EXPECT_EQ(latestMessage.error_code, 12);
	EXPECT_EQ(latestMessage.message, "Processing");
	EXPECT_NE(latestMessage.header.stamp.sec, 0);
}

}  // namespace
