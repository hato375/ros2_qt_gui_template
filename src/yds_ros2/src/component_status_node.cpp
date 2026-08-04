#include <yds/ros2/component_status_node.h>

#include <cstdint>
#include <stdexcept>

#include <rcl_interfaces/msg/parameter_descriptor.hpp>

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
	  statusPublisher_() {
	const QString componentId = QString::fromStdString(declare_parameter<std::string>(
		"component_status.component_id",
		defaultComponentId.toStdString(),
		readOnlyParameterDescriptor("コンポーネントID")));
	const QString statusTopicName = QString::fromStdString(declare_parameter<std::string>(
		"component_status.status_topic",
		defaultStatusTopicName.toStdString(),
		readOnlyParameterDescriptor("コンポーネント状態の通知トピック名")));
	const std::int64_t publishIntervalMs = declare_parameter<std::int64_t>(
		"component_status.publish_interval_ms",
		defaultPublishInterval.count(),
		readOnlyParameterDescriptor("コンポーネント状態の定期通知周期（ミリ秒）"));

	if (componentId.trimmed().isEmpty()) {
		throw std::invalid_argument("component_status.component_id must not be empty");
	}
	if (statusTopicName.trimmed().isEmpty()) {
		throw std::invalid_argument("component_status.status_topic must not be empty");
	}
	if (publishIntervalMs < kMinimumPublishIntervalMs ||
		publishIntervalMs > kMaximumPublishIntervalMs) {
		throw std::out_of_range(
			"component_status.publish_interval_ms must be between 100 and 600000");
	}
	statusPublisher_ = std::make_unique<ComponentStatusPublisher>(
		*this,
		ComponentStatusPublisherConfiguration{
			componentId,
			statusTopicName,
			std::chrono::milliseconds(publishIntervalMs)});
}

bool ComponentStatusNode::setComponentStatus(
	ComponentState state,
	qint32 errorCode,
	const QString& message) noexcept {
	return statusPublisher_->setStatus(state, errorCode, message);
}

ComponentStatus ComponentStatusNode::componentStatus() const {
	return statusPublisher_->status();
}

const QString& ComponentStatusNode::componentId() const noexcept {
	return statusPublisher_->componentId();
}

const QString& ComponentStatusNode::statusTopicName() const noexcept {
	return statusPublisher_->statusTopicName();
}

std::chrono::milliseconds ComponentStatusNode::statusPublishInterval() const noexcept {
	return statusPublisher_->publishInterval();
}

}  // namespace yds::ros2
