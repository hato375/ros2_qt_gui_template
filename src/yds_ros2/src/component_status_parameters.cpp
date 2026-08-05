#include <yds/ros2/component_status_parameters.h>

#include <cstdint>
#include <stdexcept>
#include <string>

#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp/parameter_value.hpp>

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

ComponentStatusPublisherConfiguration declareComponentStatusPublisherParameters(
	const rclcpp::node_interfaces::NodeParametersInterface::SharedPtr& nodeParameters,
	const ComponentStatusPublisherConfiguration& defaults) {
	if (!nodeParameters) {
		throw std::invalid_argument("node parameters interface must not be null");
	}

	const QString componentId = QString::fromStdString(
		nodeParameters
			->declare_parameter(
				"component_status.component_id",
				rclcpp::ParameterValue(defaults.componentId.toStdString()),
				readOnlyParameterDescriptor("コンポーネントID"))
			.get<std::string>());
	const QString statusTopicName = QString::fromStdString(
		nodeParameters
			->declare_parameter(
				"component_status.status_topic",
				rclcpp::ParameterValue(defaults.statusTopicName.toStdString()),
				readOnlyParameterDescriptor("コンポーネント状態の通知トピック名"))
			.get<std::string>());
	const std::int64_t publishIntervalMs = nodeParameters
		->declare_parameter(
			"component_status.publish_interval_ms",
			rclcpp::ParameterValue(defaults.publishInterval.count()),
			readOnlyParameterDescriptor("コンポーネント状態の定期通知周期（ミリ秒）"))
		.get<std::int64_t>();

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

	return {
		componentId,
		statusTopicName,
		std::chrono::milliseconds(publishIntervalMs)};
}

}  // namespace yds::ros2
