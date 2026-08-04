#include <yds/ros2/component_status_node.h>

#include <cstdint>
#include <stdexcept>

#include <QByteArray>
#include <QDateTime>

#include <rcl_interfaces/msg/parameter_descriptor.hpp>

#include <yds/ros2/component_status_conversion.h>

namespace {

constexpr std::int64_t kMinimumPublishIntervalMs = 100;
constexpr std::int64_t kMaximumPublishIntervalMs = 600000;

rcl_interfaces::msg::ParameterDescriptor readOnlyParameterDescriptor(
	const std::string& description) {
	rcl_interfaces::msg::ParameterDescriptor descriptor;
	descriptor.description = description;
	descriptor.read_only = true;
	return descriptor;
}

}  // namespace

namespace yds::ros2 {

ComponentStatusNode::ComponentStatusNode(
	const std::string& nodeName,
	const QString& defaultComponentId,
	const QString& defaultStatusTopicName,
	std::chrono::milliseconds defaultPublishInterval,
	const rclcpp::NodeOptions& options)
	: rclcpp::Node(nodeName, options),
	  componentId_(),
	  statusTopicName_(),
	  statusPublishInterval_(0),
	  componentStatus_{},
	  statusPublisher_(),
	  statusTimer_() {
	componentId_ = QString::fromStdString(declare_parameter<std::string>(
		"component_status.component_id",
		defaultComponentId.toStdString(),
		readOnlyParameterDescriptor("コンポーネントID")));
	statusTopicName_ = QString::fromStdString(declare_parameter<std::string>(
		"component_status.status_topic",
		defaultStatusTopicName.toStdString(),
		readOnlyParameterDescriptor("コンポーネント状態の通知トピック名")));
	const std::int64_t publishIntervalMs = declare_parameter<std::int64_t>(
		"component_status.publish_interval_ms",
		defaultPublishInterval.count(),
		readOnlyParameterDescriptor("コンポーネント状態の定期通知周期（ミリ秒）"));

	if (componentId_.trimmed().isEmpty()) {
		throw std::invalid_argument("component_status.component_id must not be empty");
	}
	if (statusTopicName_.trimmed().isEmpty()) {
		throw std::invalid_argument("component_status.status_topic must not be empty");
	}
	if (publishIntervalMs < kMinimumPublishIntervalMs ||
		publishIntervalMs > kMaximumPublishIntervalMs) {
		throw std::out_of_range(
			"component_status.publish_interval_ms must be between 100 and 600000");
	}
	statusPublishInterval_ = std::chrono::milliseconds(publishIntervalMs);

	componentStatus_ = {
		statusTopicName_,
		componentId_,
		ComponentState::kInitializing,
		0,
		QString(),
		QDateTime()};

	const auto qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
	statusPublisher_ = create_publisher<yds_interfaces::msg::ComponentStatus>(
		statusTopicName_.toStdString(), qos);
	statusTimer_ = create_wall_timer(statusPublishInterval_, [this]() {
		publishComponentStatus();
	});

	const QByteArray componentIdUtf8 = componentId_.toUtf8();
	const QByteArray topicNameUtf8 = statusTopicName_.toUtf8();
	RCLCPP_INFO(
		get_logger(),
		"Component status publisher started: component_id=%s, topic=%s, interval_ms=%ld",
		componentIdUtf8.constData(),
		topicNameUtf8.constData(),
		static_cast<long>(statusPublishInterval_.count()));

	if (!publishComponentStatus()) {
		throw std::runtime_error("Failed to publish initial component status");
	}
}

bool ComponentStatusNode::setComponentStatus(
	ComponentState state,
	qint32 errorCode,
	const QString& message) noexcept {
	try {
		{
			std::lock_guard<std::mutex> lock(statusMutex_);
			componentStatus_.state = state;
			componentStatus_.errorCode = errorCode;
			componentStatus_.message = message;
		}
		return publishComponentStatus();
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(get_logger(), "Failed to update component status: %s", exception.what());
	} catch (...) {
		RCLCPP_ERROR(get_logger(), "Failed to update component status: unknown exception");
	}
	return false;
}

ComponentStatus ComponentStatusNode::componentStatus() const {
	std::lock_guard<std::mutex> lock(statusMutex_);
	return componentStatus_;
}

const QString& ComponentStatusNode::componentId() const noexcept {
	return componentId_;
}

const QString& ComponentStatusNode::statusTopicName() const noexcept {
	return statusTopicName_;
}

std::chrono::milliseconds ComponentStatusNode::statusPublishInterval() const noexcept {
	return statusPublishInterval_;
}

bool ComponentStatusNode::publishComponentStatus() noexcept {
	try {
		ComponentStatus status;
		{
			std::lock_guard<std::mutex> lock(statusMutex_);
			componentStatus_.timestamp = QDateTime::currentDateTime();
			status = componentStatus_;
		}
		statusPublisher_->publish(componentStatusToRos(status));
		return true;
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(get_logger(), "Failed to publish component status: %s", exception.what());
	} catch (...) {
		RCLCPP_ERROR(get_logger(), "Failed to publish component status: unknown exception");
	}
	return false;
}

}  // namespace yds::ros2
