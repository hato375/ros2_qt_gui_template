#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>

#include <rclcpp/exceptions/exceptions.hpp>
#include <rclcpp/rclcpp.hpp>
#include <yds_interfaces/msg/component_status.hpp>

#include <sample_processor/sample_lifecycle_processor_node.h>
#include <yds/ros2/component_status.h>
#include <yds/ros2/topic_reception_monitor.h>

namespace {

using namespace std::chrono_literals;

class ConfigureFailureNode final
	: public sampleprocessor::SampleLifecycleProcessorNode {
public:
	using SampleLifecycleProcessorNode::SampleLifecycleProcessorNode;

protected:
	bool configureProcessor(QString& errorMessage) override {
		errorMessage = QStringLiteral("Camera connection failed");
		return false;
	}
};

class ActivateFailureNode final
	: public sampleprocessor::SampleLifecycleProcessorNode {
public:
	using SampleLifecycleProcessorNode::SampleLifecycleProcessorNode;

protected:
	bool activateProcessor(QString& errorMessage) override {
		errorMessage = QStringLiteral("Processor start failed");
		return false;
	}
};

TEST(SampleLifecycleProcessorNodeTest, RejectsInvalidProcessingInterval) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter("processing_interval_ms", 99),
	});

	EXPECT_THROW(
		std::make_shared<sampleprocessor::SampleLifecycleProcessorNode>(options),
		rclcpp::exceptions::InvalidParameterValueException);
}

TEST(SampleLifecycleProcessorNodeTest, ProcessesOnlyWhileActiveAndPublishesWhileInactive) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter("processing_interval_ms", 100),
		rclcpp::Parameter("component_status.component_id", "lifecycle-processor-test"),
		rclcpp::Parameter(
			"component_status.status_topic",
			"sample_lifecycle_processor_test/status"),
		rclcpp::Parameter("component_status.publish_interval_ms", 100),
	});
	auto node = std::make_shared<sampleprocessor::SampleLifecycleProcessorNode>(options);
	EXPECT_EQ(node->processingIntervalMs(), 100);

	node->configure();
	ASSERT_EQ(node->get_current_state().label(), "inactive");
	node->activate();
	ASSERT_EQ(node->get_current_state().label(), "active");

	auto receiverNode = std::make_shared<rclcpp::Node>("lifecycle_processor_receiver_test");
	std::mutex receivedMutex;
	std::uint64_t receivedCount = 0;
	std::uint8_t latestState = yds_interfaces::msg::ComponentStatus::STATE_UNKNOWN;
	std::string latestComponentId;
	const auto subscription =
		receiverNode->create_subscription<yds_interfaces::msg::ComponentStatus>(
			"sample_lifecycle_processor_test/status",
			rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local(),
			[&](const yds_interfaces::msg::ComponentStatus::SharedPtr message) {
				std::lock_guard<std::mutex> lock(receivedMutex);
				++receivedCount;
				latestState = message->state;
				latestComponentId = message->component_id;
			});
	ASSERT_NE(subscription, nullptr);

	rclcpp::executors::SingleThreadedExecutor executor;
	executor.add_node(node->get_node_base_interface());
	executor.add_node(receiverNode);
	std::thread executorThread([&executor]() {
		executor.spin();
	});

	const auto processingDeadline = std::chrono::steady_clock::now() + 2s;
	while (node->processedCount() == 0 &&
		std::chrono::steady_clock::now() < processingDeadline) {
		std::this_thread::sleep_for(10ms);
	}
	EXPECT_GE(node->processedCount(), 1U);

	node->deactivate();
	EXPECT_EQ(node->get_current_state().label(), "inactive");
	const std::uint64_t processedCountAfterDeactivate = node->processedCount();
	std::uint64_t receivedCountAfterDeactivate = 0;
	{
		std::lock_guard<std::mutex> lock(receivedMutex);
		receivedCountAfterDeactivate = receivedCount;
	}

	const auto heartbeatDeadline = std::chrono::steady_clock::now() + 2s;
	while (std::chrono::steady_clock::now() < heartbeatDeadline) {
		{
			std::lock_guard<std::mutex> lock(receivedMutex);
			if (receivedCount > receivedCountAfterDeactivate &&
				latestState == yds_interfaces::msg::ComponentStatus::STATE_STOPPED) {
				break;
			}
		}
		std::this_thread::sleep_for(10ms);
	}
	std::this_thread::sleep_for(150ms);
	executor.cancel();
	executorThread.join();

	EXPECT_EQ(node->processedCount(), processedCountAfterDeactivate);
	std::lock_guard<std::mutex> lock(receivedMutex);
	EXPECT_GT(receivedCount, receivedCountAfterDeactivate);
	EXPECT_EQ(latestState, yds_interfaces::msg::ComponentStatus::STATE_STOPPED);
	EXPECT_EQ(latestComponentId, "lifecycle-processor-test");
}

TEST(SampleLifecycleProcessorNodeTest, ReportsConfigureFailureAsComponentError) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter(
			"component_status.status_topic",
			"sample_lifecycle_processor_error_test/status"),
		rclcpp::Parameter("component_status.publish_interval_ms", 100),
	});
	auto node = std::make_shared<ConfigureFailureNode>(options);
	auto receiverNode = std::make_shared<rclcpp::Node>(
		"lifecycle_processor_error_receiver_test");
	yds::ros2::TopicReceptionMonitor receptionMonitor(
		QStringLiteral("sample_lifecycle_processor_error_test/status"),
		250ms);
	std::mutex receivedMutex;
	yds_interfaces::msg::ComponentStatus latestMessage;
	std::uint64_t receivedCount = 0;
	const auto subscription =
		receiverNode->create_subscription<yds_interfaces::msg::ComponentStatus>(
			"sample_lifecycle_processor_error_test/status",
			rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local(),
			[&](const yds_interfaces::msg::ComponentStatus::SharedPtr message) {
				std::lock_guard<std::mutex> lock(receivedMutex);
				latestMessage = *message;
				++receivedCount;
				receptionMonitor.recordReception(QString::fromStdString(message->message));
			});
	ASSERT_NE(subscription, nullptr);

	node->configure();
	EXPECT_EQ(node->get_current_state().label(), "unconfigured");
	const yds::ros2::ComponentStatus status = node->componentStatus();
	EXPECT_EQ(status.state, yds::ros2::ComponentState::kError);
	EXPECT_EQ(status.errorCode, 9101);
	EXPECT_EQ(status.message, QStringLiteral("Camera connection failed"));

	rclcpp::executors::SingleThreadedExecutor executor;
	executor.add_node(node->get_node_base_interface());
	executor.add_node(receiverNode);
	std::thread executorThread([&executor]() {
		executor.spin();
	});

	const auto heartbeatDeadline = std::chrono::steady_clock::now() + 2s;
	while (std::chrono::steady_clock::now() < heartbeatDeadline) {
		{
			std::lock_guard<std::mutex> lock(receivedMutex);
			if (receivedCount >= 3) {
				break;
			}
		}
		std::this_thread::sleep_for(10ms);
	}
	executor.cancel();
	executorThread.join();

	std::lock_guard<std::mutex> lock(receivedMutex);
	EXPECT_GE(receivedCount, 3U);
	EXPECT_EQ(latestMessage.state, yds_interfaces::msg::ComponentStatus::STATE_ERROR);
	EXPECT_EQ(latestMessage.error_code, 9101);
	EXPECT_EQ(latestMessage.message, "Camera connection failed");
	EXPECT_EQ(
		receptionMonitor.status().state,
		yds::ros2::TopicReceptionState::kReceiving);
	EXPECT_EQ(receptionMonitor.checkTimeout(), yds::ros2::TopicReceptionTransition::kNone);
}

TEST(SampleLifecycleProcessorNodeTest, ReportsActivateFailureAsComponentError) {
	auto node = std::make_shared<ActivateFailureNode>();

	node->configure();
	ASSERT_EQ(node->get_current_state().label(), "inactive");
	node->activate();

	EXPECT_EQ(node->get_current_state().label(), "unconfigured");
	const yds::ros2::ComponentStatus status = node->componentStatus();
	EXPECT_EQ(status.state, yds::ros2::ComponentState::kError);
	EXPECT_EQ(status.errorCode, 9102);
	EXPECT_EQ(status.message, QStringLiteral("Processor start failed"));
}

TEST(SampleLifecycleProcessorNodeTest, UpdatesStatusForCleanupAndShutdown) {
	auto node = std::make_shared<sampleprocessor::SampleLifecycleProcessorNode>();

	node->configure();
	ASSERT_EQ(node->get_current_state().label(), "inactive");
	node->cleanup();
	EXPECT_EQ(node->get_current_state().label(), "unconfigured");
	EXPECT_EQ(
		node->componentStatus().state,
		yds::ros2::ComponentState::kInitializing);
	EXPECT_EQ(node->processedCount(), 0U);

	node->shutdown();
	EXPECT_EQ(node->get_current_state().label(), "finalized");
	EXPECT_EQ(
		node->componentStatus().state,
		yds::ros2::ComponentState::kStopped);
}

}  // namespace
