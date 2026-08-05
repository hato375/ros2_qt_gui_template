#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>
#include <QString>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <yds_interfaces/msg/component_status.hpp>

#include <yds/ros2/component_status_node.h>
#include <yds/ros2/component_status_parameters.h>
#include <yds/ros2/component_status_publisher.h>
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

class TestLifecycleComponentStatusNode final : public rclcpp_lifecycle::LifecycleNode {
public:
	explicit TestLifecycleComponentStatusNode(
		const rclcpp::NodeOptions& options = rclcpp::NodeOptions())
		: rclcpp_lifecycle::LifecycleNode("lifecycle_component_status_node_test", options),
		  statusPublisher_(
			  *this,
			  yds::ros2::declareComponentStatusPublisherParameters(
				  *this,
				  {
					  QStringLiteral("lifecycle-camera-1"),
					  QStringLiteral("lifecycle_camera/status"),
					  100ms})) {}

	bool setComponentStatus(yds::ros2::ComponentState state) noexcept {
		return statusPublisher_.setStatus(state);
	}

	const QString& componentId() const noexcept {
		return statusPublisher_.componentId();
	}

	const QString& statusTopicName() const noexcept {
		return statusPublisher_.statusTopicName();
	}

	std::chrono::milliseconds statusPublishInterval() const noexcept {
		return statusPublisher_.publishInterval();
	}

private:
	yds::ros2::ComponentStatusPublisher statusPublisher_;
};

TEST(ComponentStatusPublisherTest, AddsStatusPublishingToRegularNode) {
	auto node = std::make_shared<rclcpp::Node>("component_status_publisher_test");
	yds::ros2::ComponentStatusPublisher publisher(
		*node,
		{
			QStringLiteral("circle-detector-1"),
			QStringLiteral("circle_detector/status"),
			250ms});

	EXPECT_EQ(publisher.componentId(), QStringLiteral("circle-detector-1"));
	EXPECT_EQ(publisher.statusTopicName(), QStringLiteral("circle_detector/status"));
	EXPECT_EQ(publisher.publishInterval(), 250ms);
	EXPECT_EQ(publisher.status().state, yds::ros2::ComponentState::kInitializing);
	EXPECT_TRUE(publisher.setStatus(
		yds::ros2::ComponentState::kRunning,
		0,
		QStringLiteral("Detecting circles")));
	EXPECT_EQ(publisher.status().state, yds::ros2::ComponentState::kRunning);
	EXPECT_EQ(publisher.status().message, QStringLiteral("Detecting circles"));
}

TEST(ComponentStatusPublisherTest, RejectsInvalidConfiguration) {
	auto node = std::make_shared<rclcpp::Node>("component_status_publisher_invalid_test");

	EXPECT_THROW(
		yds::ros2::ComponentStatusPublisher(
			*node,
			{QString(), QStringLiteral("status"), 1000ms}),
		std::invalid_argument);
	EXPECT_THROW(
		yds::ros2::ComponentStatusPublisher(
			*node,
			{QStringLiteral("component"), QString(), 1000ms}),
		std::invalid_argument);
	EXPECT_THROW(
		yds::ros2::ComponentStatusPublisher(
			*node,
			{QStringLiteral("component"), QStringLiteral("status"), 99ms}),
		std::out_of_range);
}

TEST(ComponentStatusPublisherTest, PublishesHeartbeatWhileLifecycleNodeIsInactive) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter("component_status.component_id", "lifecycle-camera-2"),
		rclcpp::Parameter("component_status.status_topic", "lifecycle_camera_2/status"),
		rclcpp::Parameter("component_status.publish_interval_ms", 150)});
	auto lifecycleNode = std::make_shared<TestLifecycleComponentStatusNode>(options);
	EXPECT_EQ(lifecycleNode->componentId(), QStringLiteral("lifecycle-camera-2"));
	EXPECT_EQ(lifecycleNode->statusTopicName(), QStringLiteral("lifecycle_camera_2/status"));
	EXPECT_EQ(lifecycleNode->statusPublishInterval(), 150ms);
	lifecycleNode->configure();
	lifecycleNode->activate();
	lifecycleNode->deactivate();
	ASSERT_EQ(lifecycleNode->get_current_state().label(), "inactive");

	auto receiverNode = std::make_shared<rclcpp::Node>("lifecycle_status_receiver_test");
	std::mutex receivedMutex;
	int receivedCount = 0;
	yds_interfaces::msg::ComponentStatus latestMessage;
	const auto subscription =
		receiverNode->create_subscription<yds_interfaces::msg::ComponentStatus>(
			"lifecycle_camera_2/status",
			rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local(),
			[&](const yds_interfaces::msg::ComponentStatus::SharedPtr message) {
				std::lock_guard<std::mutex> lock(receivedMutex);
				latestMessage = *message;
				++receivedCount;
			});
	ASSERT_NE(subscription, nullptr);

	rclcpp::executors::SingleThreadedExecutor executor;
	executor.add_node(lifecycleNode->get_node_base_interface());
	executor.add_node(receiverNode);
	std::thread executorThread([&executor]() {
		executor.spin();
	});

	EXPECT_TRUE(lifecycleNode->setComponentStatus(yds::ros2::ComponentState::kReady));
	const auto deadline = std::chrono::steady_clock::now() + 2s;
	while (std::chrono::steady_clock::now() < deadline) {
		{
			std::lock_guard<std::mutex> lock(receivedMutex);
			if (receivedCount >= 2) {
				break;
			}
		}
		std::this_thread::sleep_for(10ms);
	}
	executor.cancel();
	executorThread.join();

	std::lock_guard<std::mutex> lock(receivedMutex);
	EXPECT_GE(receivedCount, 2);
	EXPECT_EQ(latestMessage.component_id, "lifecycle-camera-2");
	EXPECT_EQ(latestMessage.state, yds_interfaces::msg::ComponentStatus::STATE_READY);
}

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
		QStringLiteral("画像を処理しています")));
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
	EXPECT_EQ(latestMessage.message, std::string(u8"画像を処理しています"));
	EXPECT_NE(latestMessage.header.stamp.sec, 0);
}

}  // namespace
